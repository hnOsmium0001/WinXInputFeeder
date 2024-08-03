#pragma once

#include "modelconfig.hpp"

#include <ViGEm/Client.h>

#include <cassert>
#include <minwindef.h>
#include <string_view>
#include <span>
#include <vector>

struct ViGEm {
	PVIGEM_CLIENT hvigem;

	ViGEm();
	~ViGEm();

	ViGEm(const ViGEm&) = delete;
	ViGEm& operator=(const ViGEm&) = delete;
	ViGEm(ViGEm&&) noexcept;
	ViGEm& operator=(ViGEm&&) noexcept;
};

struct X360Gamepad {
	PVIGEM_CLIENT hvigem;
	PVIGEM_TARGET htarget;

	// If == INVALID_HANDLE_VALUE, accept any input source
	// Otherwise accept only the specified input source
	HANDLE srcKbd = INVALID_HANDLE_VALUE;
	HANDLE srcMouse = INVALID_HANDLE_VALUE;

	float accuMouseX = 0.0f;
	float accuMouseY = 0.0f;
	float lastAngle = 0.0f;
	XUSB_REPORT state = {};

	X360Button pendingRebindBtn = X360Button::None;
	BYTE stickKeys = 0;
	bool pendingRebindKbd = false;
	bool pendingRebindMouse = false;

	X360Gamepad(const ViGEm& client);
	~X360Gamepad();

	X360Gamepad(const X360Gamepad&) = delete;
	X360Gamepad& operator=(const X360Gamepad&) = delete;
	X360Gamepad(X360Gamepad&&) noexcept;
	X360Gamepad& operator=(X360Gamepad&&) noexcept;

	bool GetButton(XUSB_BUTTON) const noexcept;
	void SetButton(XUSB_BUTTON, bool onoff) noexcept;

	void SetLeftTrigger(BYTE val) noexcept { state.bLeftTrigger = val; }
	void SetRightTrigger(BYTE val) noexcept { state.bRightTrigger = val; }
	void SetStickLX(SHORT val) noexcept { state.sThumbLX = val; }
	void SetStickLY(SHORT val) noexcept { state.sThumbLY = val; }
	void SetStickRX(SHORT val) noexcept { state.sThumbLX = val; }
	void SetStickRY(SHORT val) noexcept { state.sThumbRY = val; }

	void SendReport();
};

// Information and lookup tables computable from a Config object
// used for translating input key presses/mouse movements into gamepad state
struct InputTranslationStruct {
	// VK_xxx is BYTE, max 255 values
	// NOTE: XiButton::COUNT is used to indicate "this mapping is not bound"
	X360Button btns[4][0xFF];

	InputTranslationStruct() {
		ClearAll();
	}

	void ClearAll();
	void PopulateBtnLut(int userIndex, const ConfigGamepad& gamepad);
};

class FeederEngine {
private:
	ViGEm* vigem;
	Config config;

	HWND eventHwnd;
	UINT_PTR mouseCheckTimer;

	Config::ProfileRefMut currentProfile = nullptr;
	std::vector<X360Gamepad> x360s;
	//std::vector<DualShockGamepad> dualshocks;
	InputTranslationStruct its;

	bool configDirty = false;

public:
	FeederEngine(HWND eventHwnd, Config config, ViGEm& vigem);
	~FeederEngine();

	FeederEngine(const FeederEngine&) = delete;
	FeederEngine& operator=(const FeederEngine&) = delete;
	FeederEngine(FeederEngine&&) = delete;
	FeederEngine& operator=(FeederEngine&&) = delete;

	const Config& GetConfig() const { return config; }

	Config::ProfileRef GetCurrentProfile() const { return currentProfile; }
	void SelectProfile(Config::ProfileRef profile);
	bool AddProfile(std::string profileName);
	void RemoveProfile(Config::ProfileRef profile);

	std::span<const X360Gamepad> GetX360s() const { return x360s; }
	bool AddX360();
	bool RemoveGamepad(int gamepadId);

	void StartRebindX360Device(int gamepadId, IdevKind kind);
	void RebindX360Device(int gamepadId, IdevKind kind, HANDLE);

	void StartRebindX360Mapping(int gamepadId, X360Button btn);
	// TODO set joystick options in mouse mode
	void SetX360JoystickMode(int gamepadId, bool useRight /* false: left */, bool useMouse /* false: keyboard */);

	void HandleKeyPress(HANDLE hDevice, BYTE vkey, bool pressed);
	void HandleMouseMovement(HANDLE hDevice, LONG dx, LONG dy);
	// Send joystick state generated from mouse to ViGEm
	// Triggered on a timer
	void Update();
};
