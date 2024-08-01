#pragma once

#include <ViGEm/Client.h>

#include <cassert>
#include <string_view>
#include <minwindef.h>

struct ViGEm {
	PVIGEM_CLIENT hvigem;

	ViGEm();
	~ViGEm();

	ViGEm(const ViGEm&) = delete;
	ViGEm& operator=(const ViGEm&) = delete;
	ViGEm(ViGEm&&) noexcept;
	ViGEm& operator=(ViGEm&&) noexcept;
};

enum class X360Button : unsigned char {
	// These corresponds to a XUSB_GAMEPAD_* enum
	// Compute (1 << btn) to get the XUSB_GAMEPAD_* counterpart
	/* XUSB_GAMEPAD_DPAD_UP */        DPadUp,
	/* XUSB_GAMEPAD_DPAD_DOWN */      DPadDown,
	/* XUSB_GAMEPAD_DPAD_LEFT */      DPadLeft,
	/* XUSB_GAMEPAD_DPAD_RIGHT */     DPadRight,
	/* XUSB_GAMEPAD_START */          Start,
	/* XUSB_GAMEPAD_BACK */           Back,
	/* XUSB_GAMEPAD_LEFT_THUMB */     LeftThumb,
	/* XUSB_GAMEPAD_RIGHT_THUMB */    RightThumb,
	/* XUSB_GAMEPAD_LEFT_SHOULDER */  LeftShoulder,
	/* XUSB_GAMEPAD_RIGHT_SHOULDER */ RightShoulder,
	/* XUSB_GAMEPAD_GUIDE */          Guide,
	DUMMY,
	/* XUSB_GAMEPAD_A */              A,
	/* XUSB_GAMEPAD_B */              B,
	/* XUSB_GAMEPAD_X */              X,
	/* XUSB_GAMEPAD_Y */              Y,

	DIRECT_MAP_COUNT,

	// These are exclusively used by ourselves, for mapping a key to an analog value
	LeftTrigger = DIRECT_MAP_COUNT,
	RightTrigger,

	STICK_BEGIN,
	LStickUp = STICK_BEGIN, LStickDown, LStickLeft, LStickRight,
	RStickUp, RStickDown, RStickLeft, RStickRight,
	STICK_END,

	TOTAL_COUNT = STICK_END,
	None = TOTAL_COUNT,
};

constexpr auto kX360ButtonCount = static_cast<unsigned char>(X360Button::TOTAL_COUNT);
constexpr auto kX360ButtonDirectMapCount = static_cast<unsigned char>(X360Button::DIRECT_MAP_COUNT);
constexpr auto kX360ButtonAnalogCounterpartCount = kX360ButtonCount - kX360ButtonDirectMapCount;

X360Button X360ButtonFromViGEm(XUSB_BUTTON) noexcept;
XUSB_BUTTON X360ButtonToViGEm(X360Button) noexcept;
std::string_view X360ButtonToString(X360Button) noexcept;

inline bool IsX360ButtonStick(X360Button btn) noexcept {
	auto begin = static_cast<unsigned char>(X360Button::STICK_BEGIN);
	auto end = static_cast<unsigned char>(X360Button::STICK_END);
	auto i = static_cast<unsigned char>(btn);
	return begin <= i && i < end;
}

inline bool IsX360ButtonValid(X360Button btn) noexcept {
	return static_cast<unsigned char>(btn) < kX360ButtonCount;
}
inline bool IsX360ButtonDirectMap(X360Button btn) noexcept {
	assert(IsX360ButtonValid(btn));
	return static_cast<unsigned char>(btn) < kX360ButtonDirectMapCount;
}
inline bool IsX360ButtonAnalogCounterpart(X360Button btn) noexcept {
	assert(IsX360ButtonValid(btn));
	return !IsX360ButtonDirectMap(btn);
}

struct X360Gamepad {
	PVIGEM_CLIENT hvigem;
	PVIGEM_TARGET htarget;

	// If == INVALID_HANDLE_VALUE, accept any input source
	// Otherwise accept only the specified input source
	HANDLE srcKbd = INVALID_HANDLE_VALUE;
	HANDLE srcMouse = INVALID_HANDLE_VALUE;

	XUSB_REPORT state = {};

	X360Button pendingRebindBtn = X360Button::None;
	bool pendingRebindDevice = false;

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
