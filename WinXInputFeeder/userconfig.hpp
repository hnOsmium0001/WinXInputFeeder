#pragma once

#include "gamepad.hpp"
#include "inputdevice.hpp"
#include "utils.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <span>
#include <string_view>
#include <toml++/toml.h>

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
};

struct Config {
	using ProfileTable = std::map<std::string, ConfigProfile, std::less<>>;
	using ProfileRef = ProfileTable::value_type*;

	ProfileTable profiles;
	// Recommends 50-100
	int mouseCheckFrequency = 75;
	KeyCode hotkeyShowUI;
	KeyCode hotkeyCaptureCursor;
};

toml::table SaveConfig(const Config&);
Config LoadConfig(const toml::table&);
