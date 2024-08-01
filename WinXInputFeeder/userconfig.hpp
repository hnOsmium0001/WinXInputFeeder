#pragma once

#include "gamepad.hpp"
#include "inputdevice.hpp"
#include "utils.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <toml++/toml.h>

struct Joystick {
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

struct UserProfile {
	KeyCode buttons[kX360ButtonCount];
	Joystick lstick, rstick;
};

struct Config {
	using ProfileTable = std::map<std::string, UserProfile, std::less<>>;
	using ProfileRef = ProfileTable::value_type*;

	ProfileTable profiles;
	ProfileRef x360s[4];
	//std::vector<ProfileRef> dualshocks;
	int x360Count = 0;
	// Recommends 50-100
	int mouseCheckFrequency = 75;
	KeyCode hotkeyShowUI;
	KeyCode hotkeyCaptureCursor;
};

toml::table SaveConfig(const Config&);
Config LoadConfig(const toml::table&);
