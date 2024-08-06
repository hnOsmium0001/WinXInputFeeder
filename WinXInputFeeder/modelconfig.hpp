#pragma once

#include "inputdevice.hpp"
#include "utils.hpp"

#include <ViGEm/Client.h>

#include <map>
#include <string>
#include <span>
#include <string>
#include <string_view>
#include <toml++/toml.h>
#include <vector>

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

struct ConfigJoystick {
	/* Settings for keyboard mode */
	float speed = 1.0f;

	/* Settings for mouse mode */
	// Lower value corresponds to higher sensitivity
	float sensitivity = 15.0f;
	// 1.0 is linear
	// < 1 makes center more sensitive
	float nonLinear = 1.0f;
	// Range: [0,1]
	float deadzone = 0.0f;
	bool invertXAxis = false;
	bool invertYAxis = false;

	// If true, both axis will be generated from mouse movements (specifically the mouse specified by XiGamepad.srcMouse)
	bool useMouse = false;
};

struct ConfigGamepad {
	KeyCode buttons[kX360ButtonCount];
	ConfigJoystick lstick, rstick;

	ConfigGamepad();
};

struct ConfigProfile {
	std::vector<ConfigGamepad> gamepads;
	unsigned char x360Count = 0; // Max 4

	size_t GetX360Count() const { return x360Count; }
	std::span<ConfigGamepad> GetX360s() { return std::span(gamepads.data(), x360Count); }
	// Fail: if returned size_t is max value
	std::pair<ConfigGamepad&, size_t> AddX360();

	size_t GetDS4Count() const { return gamepads.size() - x360Count; }
	std::span<ConfigGamepad> GetDS4s() { return std::span(gamepads.data() + x360Count, GetDS4Count()); }
	std::pair<ConfigGamepad&, size_t> AddDS4();

	void RemoveGamepad(size_t idx);
};

struct Config {
	using ProfileTable = std::map<std::string, ConfigProfile, std::less<>>;
	using ProfileRef = const ProfileTable::value_type*;
	using ProfileRefMut = ProfileTable::value_type*;

	ProfileTable profiles;
	// Recommends 50-100
	int mouseCheckFrequency = 75;
	KeyCode hotkeyShowUI = 0xFF;
	KeyCode hotkeyCaptureCursor = 0xFF;

	Config();
	Config(const toml::table&);

	toml::table ExportAsToml() const;
};
