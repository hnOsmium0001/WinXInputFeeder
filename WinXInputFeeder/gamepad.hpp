#pragma once

#include "userconfig.hpp"
#include "inputdevice.hpp"

#include <ViGEm/Client.h>

#include <minwindef.h>

struct ViGEm {
	PVIGEM_CLIENT hvigem;

	ViGEm();
	~ViGEm();

	ViGEm(const ViGEm&) = delete;
	ViGEm& operator=(const ViGEm&) = delete;
	ViGEm(ViGEm&&);
	ViGEm& operator=(ViGEm&&);
};

enum class XiButton : unsigned char {
	None = 0,
	A, B, X, Y,
	LB, RB,
	LT, RT,
	Start, Back,
	DpadUp, DpadDown, DpadLeft, DpadRight,
	LStickBtn, RStickBtn,
	LStickUp, LStickDown, LStickLeft, LStickRight,
	RStickUp, RStickDown, RStickLeft, RStickRight,
};

struct X360Gamepad {
	PVIGEM_CLIENT hvigem;
	PVIGEM_TARGET htarget;

	// If == INVALID_HANDLE_VALUE, accept any input source
	// Otherwise accept only the specified input source
	HANDLE srcKbd = INVALID_HANDLE_VALUE;
	HANDLE srcMouse = INVALID_HANDLE_VALUE;

	XUSB_REPORT state = {};

	XiButton pendingRebindBtn = XiButton::None;
	bool pendingRebindDevice = false;

	X360Gamepad(const ViGEm& client);
	~X360Gamepad();

	X360Gamepad(const X360Gamepad&) = delete;
	X360Gamepad& operator=(const X360Gamepad&) = delete;
	X360Gamepad(X360Gamepad&&);
	X360Gamepad& operator=(X360Gamepad&&);

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
