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

static void CALLBACK MouseCheckTimeProc(HWND hWnd, UINT message, UINT_PTR idTimer, DWORD dwTime) {
	auto self = reinterpret_cast<FeederEngine*>(idTimer);
	self->Update();
}

FeederEngine::FeederEngine(HWND eventHwnd, Config c, ViGEm& vigem)
	: vigem{ &vigem }
	, config{ std::move(c) }
	, eventHwnd{ eventHwnd }
{
	if (!config.profiles.empty())
		SelectProfile(&*config.profiles.begin());

	mouseCheckTimer = SetTimer(eventHwnd, reinterpret_cast<UINT_PTR>(this), config.mouseCheckFrequency, MouseCheckTimeProc);
	if (!mouseCheckTimer)
		LOG_DEBUG(L"Failed to register mouse check timer");
}

FeederEngine::~FeederEngine() {
	KillTimer(eventHwnd, mouseCheckTimer);
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
	if (gamepadId < 0 || gamepadId >= x360s.size())
		return false;
	currentProfile->second.RemoveGamepad(gamepadId);
	if (gamepadId < x360s.size())
		x360s.erase(x360s.begin() + gamepadId);
	else
		; // TODO
	return true;
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
			if (dev.pendingRebindMouse) {
				dev.srcMouse = hDevice;
				dev.pendingRebindMouse = false;
			}

			if (dev.srcMouse != hDevice) continue;
		}
		else {
			if (dev.pendingRebindKbd) {
				dev.srcKbd = hDevice;
				dev.pendingRebindKbd = false;
			}

			if (dev.srcKbd != hDevice) continue;
		}

		// Handle button rebinds
		if (dev.pendingRebindBtn != None) {
			gamepad.buttons[std::to_underlying(dev.pendingRebindBtn)] = vkey;
			its.PopulateBtnLut(gamepadId, gamepad);

			dev.pendingRebindBtn = None;
		}

		// Bits in X360Gamepad::stickKeys corresponding to each button
		enum { LUp, LDown, LLeft, LRight, RUp, RDown, RLeft, RRight };

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

#define SET_BIT(nth) SetUnsetBit(dev.stickKeys, nth, pressed)
#define HAS_BIT(nth) (dev.stickKeys & (1 << nth))
		case LStickUp: SET_BIT(LUp); goto lstick;
		case LStickDown: SET_BIT(LDown); goto lstick;
		case LStickLeft: SET_BIT(LLeft); goto lstick;
		case LStickRight: SET_BIT(LRight); goto lstick;
		lstick: {
			auto val = static_cast<SHORT>(MAXSHORT * gamepad.lstick.speed);
			dev.SetStickLX((HAS_BIT(LRight) ? val : 0) - (HAS_BIT(LLeft) ? val : 0));
			dev.SetStickLY((HAS_BIT(LUp) ? val : 0) - (HAS_BIT(LDown) ? val : 0));
		} break;

		case RStickUp: SET_BIT(RUp); goto rstick;
		case RStickDown: SET_BIT(RDown); goto rstick;
		case RStickLeft: SET_BIT(RLeft); goto rstick;
		case RStickRight: SET_BIT(RRight); goto rstick;
		rstick: {
			auto val = static_cast<SHORT>(MAXSHORT * gamepad.lstick.speed);
			dev.SetStickRX((HAS_BIT(RRight) ? val : 0) - (HAS_BIT(RLeft) ? val : 0));
			dev.SetStickRY((HAS_BIT(RUp) ? val : 0) - (HAS_BIT(RDown) ? val : 0));
		} break;
#undef HAS_BIT
#undef SET_BIT
		}

		dev.SendReport();
	}
}

static float Scale(float x, float lowerbound, float upperbound) {
	return (x - lowerbound) / (upperbound - lowerbound);
}

constexpr float pi = 3.14159265358979323846f;

/// \param phi phi ∈ [0,2π], defines in which direction the stick is tilted.
/// \param tilt tilt ∈ (0,1], defines the amount of tilt. 0 is no tilt, 1 is full tilt.
static void CalcJoystickPosition(float phi, float tilt, bool invertX, bool invertY, short& outX, short& outY) {
	constexpr float kSnapToFullFilt = 0.005f;

	tilt = std::clamp(tilt, 0.0f, 1.0f);
	tilt = (1 - tilt) < kSnapToFullFilt ? 1 : tilt;

#define UNNORMALIZE(VAL) static_cast<short>(VAL * 32767) 

#define STICK_MORE_VERTI(LOWERBOUND, UPPERBOUND, X_DIR, Y_DIR) \
	if (phi >= LOWERBOUND && phi <= UPPERBOUND) { \
		outX = UNNORMALIZE(X_DIR * tilt * Scale(phi, LOWERBOUND, UPPERBOUND)); \
		outY = UNNORMALIZE(Y_DIR * tilt); \
		return; \
	}
#define STICK_MORE_HORIZ(LOWERBOUND, UPPERBOUND, X_DIR, Y_DIR) \
	if (phi >= LOWERBOUND && phi <= UPPERBOUND) { \
		outX = UNNORMALIZE(X_DIR * tilt); \
		outY = UNNORMALIZE(Y_DIR * tilt * Scale(phi, LOWERBOUND, UPPERBOUND)); \
		return; \
	}

#define POS_X (invertX ? -1 : 1)
#define NEG_X (invertX ? 1 : -1)
#define POS_Y (invertY ? -1 : 1)
#define NEG_Y (invertY ? 1 : -1)

	// Two cases with forward+right
	// Tilt is forward and slightly right
	STICK_MORE_VERTI(3*pi/2, 7*pi/4, POS_X, POS_Y);
	// Tilt is slightly forward and right.
	STICK_MORE_HORIZ(7*pi/4, 2*pi, POS_X, POS_Y);

	// Two cases with right+downward
	// Tilt is right and slightly downward.
	STICK_MORE_HORIZ(0, pi/4, POS_X, NEG_Y);
	// Tilt is downward and slightly right.
	STICK_MORE_VERTI(pi/4, pi/2, POS_X, NEG_Y);

	// Two cases with downward+left
	// Tilt is downward and slightly left.
	STICK_MORE_VERTI(pi/2, 3*pi/4, NEG_X, NEG_Y);
	// Tilt is left and slightly downward.
	STICK_MORE_VERTI(3*pi/4, pi, NEG_X, NEG_Y);

	// Two cases with forward+left
	// Tilt is left and slightly forward.
	STICK_MORE_HORIZ(pi, 5*pi/4, NEG_X, POS_Y);
	// Tilt is forward and slightly left.
	STICK_MORE_VERTI(5*pi/4, 3*pi/2, NEG_X, POS_Y);

	// If nothing matched, reset stick
	outX = 0;
	outY = 0;
}

// A variant of atan2() that spits out [0,2π)
static float Atan2Positive(float x, float y) {
	if (x == 0.0f)
		if (y > 0.0f)
			return pi/2;
		else
			return 3*pi/2;
	float phi = atan(y/x);
	if (x < 0 && y > 0)
		return phi + pi;
	if (x < 0 && y <= 0)
		return phi + pi;
	if (x > 0 && y < 0)
		return phi + 2*pi;
	/* if (x > 0 && y >= 0) */
	return phi;
}

void FeederEngine::HandleMouseMovement(HANDLE hDevice, LONG dx, LONG dy) {
	for (int gamepadId = 0; gamepadId < x360s.size(); ++gamepadId) {
		auto& dev = x360s[gamepadId];
		if (dev.srcMouse != hDevice) continue;

		dev.accuMouseX += dx;
		dev.accuMouseY += dy;
	}
}

void FeederEngine::Update() {
	constexpr float kOuterRadius = 10.0f;
	constexpr float kBounceBack = 0.0f;

	for (int gamepadId = 0; gamepadId < x360s.size(); ++gamepadId) {
		auto& gamepad = currentProfile->second.gamepads[gamepadId];
		auto& dev = x360s[gamepadId];

		// Skip expensive calculations if both sticks don't use mouse2joystick
		if (!gamepad.lstick.useMouse && !gamepad.rstick.useMouse)
			continue;

		float accuX = dev.accuMouseX;
		float accuY = dev.accuMouseY;

		// Distance of mouse from center
		float r = sqrt(accuX * accuX + accuY * accuY);

		// Clamp to a point on controller circle, if we are outside it
		//if (r > kOuterRadius) {
		//	accuX = round(accuX * (kOuterRadius - kBounceBack) / r);
		//	accuX = round(accuY * (kOuterRadius - kBounceBack) / r);
		//	r = sqrt(accuX * accuX + accuY * accuY);
		//}

		float phi = Atan2Positive(accuY, accuX);
		dev.lastAngle = phi; //DBG

		auto forStick = [&](const ConfigJoystick& conf, short& outX, short& outY) {
			if (!conf.useMouse)
				return;
			if (r > conf.deadzone * kOuterRadius) {
				float num = r - conf.deadzone * kOuterRadius;
				float denom = kOuterRadius - conf.deadzone * kOuterRadius;
				CalcJoystickPosition(phi, pow(num / denom, conf.nonLinear), conf.invertXAxis, conf.invertYAxis, outX, outY);
			}
			else {
				outX = 0;
				outY = 0;
			}};
		forStick(gamepad.lstick, dev.state.sThumbLX, dev.state.sThumbLY);
		forStick(gamepad.rstick, dev.state.sThumbRX, dev.state.sThumbRY);

		dev.accuMouseX = 0.0f;
		dev.accuMouseY = 0.0f;

		dev.SendReport();
	}
}
