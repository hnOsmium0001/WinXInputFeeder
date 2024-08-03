#include "pch.hpp"

#include "modelruntime.hpp"

#include <format>
#include <stdexcept>
#include <utility>

using namespace std::literals;

ViGEm::ViGEm()
	: hvigem{ vigem_alloc() }
{
	VIGEM_ERROR err;

	err = vigem_connect(hvigem);
	if (!VIGEM_SUCCESS(err))
		throw std::runtime_error(std::format("Failed to connect ViGEm bus"));
}

ViGEm::~ViGEm() {
	vigem_disconnect(hvigem);
	vigem_free(hvigem);
}

ViGEm::ViGEm(ViGEm&& that) noexcept
	: hvigem{ std::exchange(that.hvigem, nullptr) } {}

ViGEm& ViGEm::operator=(ViGEm&& that) noexcept {
	vigem_disconnect(hvigem);
	vigem_free(hvigem);
	hvigem = std::exchange(that.hvigem, nullptr);
	return *this;
}

X360Gamepad::X360Gamepad(const ViGEm& client)
	: hvigem{ client.hvigem }
	, htarget{ vigem_target_x360_alloc() }
{
	VIGEM_ERROR err;

	err = vigem_target_add(client.hvigem, htarget);
	if (!VIGEM_SUCCESS(err))
		throw std::runtime_error(std::format("Failed to add X360 gamepad to ViGEm bus"));
}

X360Gamepad::~X360Gamepad() {
	vigem_target_remove(hvigem, htarget);
	vigem_target_free(htarget);
}

X360Gamepad::X360Gamepad(X360Gamepad&& that) noexcept
	: hvigem{ std::exchange(that.hvigem, nullptr) }
	, htarget{ std::exchange(that.htarget, nullptr) } {}

X360Gamepad& X360Gamepad::operator=(X360Gamepad&& that) noexcept {
	vigem_target_remove(hvigem, htarget);
	vigem_target_free(htarget);
	hvigem = std::exchange(that.hvigem, nullptr);
	htarget = std::exchange(that.htarget, nullptr);
	return *this;
}

bool X360Gamepad::GetButton(XUSB_BUTTON btn) const noexcept {
	// When an integral value is coerced into bool, all non-zero values are turned to 1 (and zero to 0)
	return state.wButtons & btn;
}

void X360Gamepad::SetButton(XUSB_BUTTON btn, bool onoff) noexcept {
	if (onoff)
		state.wButtons |= btn;
	else
		state.wButtons &= ~btn;
}

void X360Gamepad::SendReport() {
	vigem_target_x360_update(hvigem, htarget, state);
}

void InputTranslationStruct::ClearAll() {
	for (int gamepadId = 0; gamepadId < 4; ++gamepadId) {
		for (int i = 0; i < 0xFF; ++i) {
			btns[gamepadId][i] = X360Button::None;
		}
	}
}

void InputTranslationStruct::PopulateBtnLut(int gamepadId, const ConfigGamepad& gamepad) {
	using enum X360Button;

	// Clear
	for (auto& btn : btns[gamepadId])
		btn = None;

	for (unsigned char i = 0; i < kX360ButtonCount + 2 /* triggers */; ++i) {
		auto boundKey = gamepad.buttons[i];
		if (boundKey != 0xFF)
			btns[gamepadId][boundKey] = static_cast<X360Button>(i);
	}

	auto DoStick = [&](unsigned char base, const ConfigJoystick& stick) {
		if (stick.useMouse)
			return;
		for (unsigned char i = 0; i < 4; ++i) {
			auto boundKey = gamepad.buttons[base + i];
			if (boundKey != 0xFF)
				btns[gamepadId][boundKey] = static_cast<X360Button>(base + i);
		}};
	DoStick(static_cast<unsigned char>(LStickUp), gamepad.lstick);
	DoStick(static_cast<unsigned char>(RStickUp), gamepad.rstick);
}

FeederEngine::FeederEngine(Config c, ViGEm& vigem)
	: vigem{ &vigem }
	, config{ std::move(c) }
{
	if (!config.profiles.empty())
		SelectProfile(&*config.profiles.begin());
}

void FeederEngine::SelectProfile(Config::ProfileRef profileConst) {
	auto profile = const_cast<Config::ProfileRefMut>(profileConst);

	if (currentProfile == profile)
		return;

	x360s.clear();
	its.ClearAll();

	currentProfile = profile;
	if (profile) {
		const ConfigProfile& p = profile->second;
		size_t n = p.GetX360Count();

		x360s.reserve(n);
		for (int i = 0; i < n; ++i) {
			x360s.push_back(X360Gamepad(*vigem));
			its.PopulateBtnLut(i, p.gamepads[i]);
		}
	}
}

bool FeederEngine::AddProfile(std::string profileName) {
	auto [DISCARD, success] = config.profiles.try_emplace(std::move(profileName));
	return success;
}

void FeederEngine::RemoveProfile(Config::ProfileRef profileConst) {
	auto profile = const_cast<Config::ProfileRefMut>(profileConst);

	config.profiles.erase(profile->first);
	if (currentProfile == profile) {
		if (!config.profiles.empty())
			SelectProfile(&*config.profiles.begin());
		else
			SelectProfile(nullptr);
	}
}

bool FeederEngine::AddX360() {
	auto&& [gamepad, gamepadId] = currentProfile->second.AddX360();
	if (gamepadId == SIZE_MAX)
		return false;

	x360s.push_back(X360Gamepad(*vigem));
	auto& dev = x360s.back();

	return true;
}

bool FeederEngine::RemoveGamepad(int gamepadId) {
	// TODO
	return false;
}

void FeederEngine::StartRebindX360Device(int gamepadId, IdevKind kind) {
	if (gamepadId < 0 || gamepadId >= x360s.size())
		return;
	auto& dev = x360s[gamepadId];

	using enum IdevKind;
	switch (kind) {
	case Keyboard: dev.pendingRebindKbd = true; break;
	case Mouse: dev.pendingRebindMouse = true; break;
	}
}

void FeederEngine::RebindX360Device(int gamepadId, IdevKind kind, HANDLE handle) {
	if (gamepadId < 0 || gamepadId >= x360s.size())
		return;
	auto& dev = x360s[gamepadId];

	using enum IdevKind;
	switch (kind) {
	case Keyboard: dev.srcKbd = handle; break;
	case Mouse: dev.srcMouse = handle; break;
	}
}

void FeederEngine::StartRebindX360Mapping(int gamepadId, X360Button btn) {
	if (gamepadId < 0 || gamepadId >= x360s.size())
		return;
	auto& dev = x360s[gamepadId];

	dev.pendingRebindBtn = btn;
}

void FeederEngine::SetX360JoystickMode(int gamepadId, bool useRight, bool useMouse) {
	if (gamepadId < 0 || gamepadId >= x360s.size())
		return;
	auto& gamepad = currentProfile->second.gamepads[gamepadId];
	auto& dev = x360s[gamepadId];

	auto& stick = useRight ? gamepad.rstick : gamepad.lstick;
	stick.useMouse = useMouse;
	configDirty = true;
}

void FeederEngine::HandleKeyPress(HANDLE hDevice, BYTE vkey, bool pressed) {
	using enum X360Button;
	for (int gamepadId = 0; gamepadId < x360s.size(); ++gamepadId) {
		auto& dev = x360s[gamepadId];
		auto& gamepad = currentProfile->second.gamepads[gamepadId];

		// Device filtering
		if (IsKeyCodeMouseButton(vkey)) {
			HANDLE src = dev.srcMouse;
			if (src != INVALID_HANDLE_VALUE && src != hDevice) continue;

			if (dev.pendingRebindMouse) {
				dev.srcMouse = hDevice;
				dev.pendingRebindMouse = false;
			}
		}
		else {
			HANDLE src = dev.srcKbd;
			if (src != INVALID_HANDLE_VALUE && src != hDevice) continue;

			if (dev.pendingRebindKbd) {
				dev.srcKbd = hDevice;
				dev.pendingRebindKbd = false;
			}
		}

		// Handle button rebinds
		if (dev.pendingRebindBtn != None) {
			gamepad.buttons[std::to_underlying(dev.pendingRebindBtn)] = vkey;
			its.PopulateBtnLut(gamepadId, gamepad);

			dev.pendingRebindBtn = None;
		}

		constexpr int kStickMaxVal = 32767;

		X360Button btn = its.btns[gamepadId][vkey];
		if (btn == None)
			continue;
		if (IsX360ButtonDirectMap(btn)) {
			dev.SetButton(X360ButtonToViGEm(btn), pressed);
		}
		else switch (btn) {
		case LeftTrigger: dev.SetLeftTrigger(pressed ? 0xFF : 0x00); break;
		case RightTrigger:dev.SetRightTrigger(pressed ? 0xFF : 0x00); break;

			// NOTE: we assume that if any key is setup for the joystick directions, it's on keyboard mode
			//       that is, we rely on the translation struct being populated from the current user config correctly

			// Stick's actual value per user's speed setting
			// (in config it's specified as a fraction between 0 to 1
#define STICK_VALUE static_cast<SHORT>(kStickMaxVal * gamepad.lstick.speed)
			// TODO use the setters
		case LStickUp: dev.state.sThumbLY += STICK_VALUE; break;
		case LStickDown:dev.state.sThumbLY -= STICK_VALUE; break;
		case LStickLeft: dev.state.sThumbLX -= STICK_VALUE; break;
		case LStickRight: dev.state.sThumbLX += STICK_VALUE; break;
		case RStickUp: dev.state.sThumbRY += STICK_VALUE; break;
		case RStickDown:dev.state.sThumbRY -= STICK_VALUE; break;
		case RStickLeft: dev.state.sThumbRX -= STICK_VALUE; break;
		case RStickRight: dev.state.sThumbRX += STICK_VALUE; break;
#undef STICK_VALUE
		}

		dev.SendReport();
	}
}

static float Scale(float x, float lowerbound, float upperbound) {
	return (x - lowerbound) / (upperbound - lowerbound);
}

/// \param phi phi ¡Ê [0,2¦Ð], defines in which direction the stick is tilted.
/// \param tilt tilt ¡Ê (0,1], defines the amount of tilt. 0 is no tilt, 1 is full tilt.
static void SetJoystickPosition(float phi, float tilt, bool invertX, bool invertY, short& outX, short& outY) {
	constexpr float pi = 3.14159265358979323846f;
	constexpr float kSnapToFullFilt = 0.005f;

	tilt = std::clamp(tilt, 0.0f, 1.0f);
	tilt = (1 - tilt) < kSnapToFullFilt ? 1 : tilt;

	// [0,1]
	float x, y;

#define RANGE_FOR_X(LOWERBOUND, UPPERBOUND, FACTOR) if (phi >= LOWERBOUND && phi <= UPPERBOUND) { x = FACTOR * tilt * Scale(phi, LOWERBOUND, UPPERBOUND); y = FACTOR * tilt; }
#define RANGE_FOR_Y(LOWERBOUND, UPPERBOUND, FACTOR) if (phi >= LOWERBOUND && phi <= UPPERBOUND) { x = FACTOR * tilt; y = FACTOR * tilt * Scale(phi, LOWERBOUND, UPPERBOUND); }
	// Two cases with forward+right
	// Tilt is forward and slightly right
	RANGE_FOR_X(3 * pi / 2, 7 * pi / 4, 1);
	// Tilt is slightly forwardand right.
	RANGE_FOR_Y(7 * pi / 4, 2 * pi, 1);
	// Two cases with right+downward
	// Tilt is right and slightly downward.
	RANGE_FOR_Y(0, pi / 4, 1);
	// Tilt is downward and slightly right.
	RANGE_FOR_X(pi / 4, pi / 2, 1);
	// Two cases with downward+left
	// Tilt is downward and slightly left.
	RANGE_FOR_X(pi / 2, 3 * pi / 4, -1);
	// Tilt is left and slightly downward.
	RANGE_FOR_X(3 * pi / 4, pi, -1);
	// Two cases with forward+left
	// Tilt is left and slightly forward.
	RANGE_FOR_Y(pi, 5 * pi / 4, -1);
	// Tilt is forward and slightly left.
	RANGE_FOR_X(5 * pi / 4, 3 * pi / 2, -1);
#undef RANGE_FOR_X
#undef RANGE_FOR_Y


	// Scale [0,1] to [INT16_MIN,INT16_MAX]
	outX = static_cast<short>(x * 32768);
	outY = static_cast<short>(y * 32768);

	if (invertX) outX = -outX;
	if (invertY) outY = -outY;
}

void FeederEngine::HandleMouseMovement(HANDLE hDevice, LONG dx, LONG dy) {
	for (int gamepadId = 0; gamepadId < x360s.size(); ++gamepadId) {
		auto& dev = x360s[gamepadId];
		HANDLE src = dev.srcMouse;
		if (src != INVALID_HANDLE_VALUE && src != hDevice) continue;

		dev.accuMouseX += dx;
		dev.accuMouseY += dy;
	}
}

void FeederEngine::Update() {
	for (int gamepadId = 0; gamepadId < x360s.size(); ++gamepadId) {
		auto& gamepad = currentProfile->second.gamepads[gamepadId];
		auto& dev = x360s[gamepadId];

		constexpr float kOuterRadius = 10.0f;
		constexpr float kBounceBack = 0.0f;

		float accuX = dev.accuMouseX;
		float accuY = dev.accuMouseY;
		// Distance of mouse from center
		float r = sqrt(accuX * accuX + accuY * accuY);

		// Clamp to a point on controller circle, if we are outside it
		if (r > kOuterRadius) {
			accuX = round(accuX * (kOuterRadius - kBounceBack) / r);
			accuX = round(accuY * (kOuterRadius - kBounceBack) / r);
			r = sqrt(accuX * accuX + accuY * accuY);
		}

		float phi = atan2(accuY, accuX);

		auto forStick = [&](const ConfigJoystick& conf, short& outX, short& outY) {
			if (r > conf.sensitivity * kOuterRadius) {
				float num = r - conf.sensitivity * kOuterRadius;
				float denom = kOuterRadius - conf.sensitivity * kOuterRadius;
				SetJoystickPosition(phi, pow(num / denom, conf.nonLinear), conf.invertXAxis, conf.invertYAxis, outX, outY);
			}
			else {
				outX = 0;
				outY = 0;
			}
			};
		forStick(gamepad.lstick, dev.state.sThumbLX, dev.state.sThumbLY);
		forStick(gamepad.rstick, dev.state.sThumbRX, dev.state.sThumbRY);

		dev.accuMouseX = 0.0f;
		dev.accuMouseY = 0.0f;
	}
}
