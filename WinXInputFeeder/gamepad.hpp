#pragma once

#include "userconfig.hpp"
#include "inputdevice.hpp"

#include <ViGEm/Client.h>

#include <minwindef.h>

struct X360Gamepad {
	const UserProfile* profile = nullptr;

	// If == INVALID_HANDLE_VALUE, accept any input source
	// Otherwise accept only the specified input source
	HANDLE srcKbd = INVALID_HANDLE_VALUE;
	HANDLE srcMouse = INVALID_HANDLE_VALUE;

	// This shall be incremented every time any field is updated due to input state changes
	int epoch = 0;

	short lstickX, lstickY;
	short rstickX, rstickY;
	bool a, b, x, y;
	bool lb, rb;
	bool lt, rt;
	bool start, back;
	bool dpadUp, dpadDown, dpadLeft, dpadRight;
	bool lstickBtn, rstickBtn;

	XUSB_REPORT state;

	void SetButton(XUSB_BUTTON, bool onoff) noexcept;
	void SetLeftTrigger(BYTE val) noexcept { state.bLeftTrigger = val; }
	void SetRightTrigger(BYTE val) noexcept { state.bRightTrigger = val; }
	void SetStickLX(SHORT val) noexcept { state.sThumbLX = val; }
	void SetStickLY(SHORT val) noexcept { state.sThumbLY = val; }
	void SetStickRX(SHORT val) noexcept { state.sThumbLX = val; }
	void SetStickRY(SHORT val) noexcept { state.sThumbRY = val; }
};

extern SRWLOCK gX360GamepadsLock;
extern bool gX360GamepadsEnabled[4];
extern X360Gamepad gX360Gamepads[4];
