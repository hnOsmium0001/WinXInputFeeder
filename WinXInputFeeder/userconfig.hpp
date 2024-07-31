#pragma once

#include "inputdevice.hpp"
#include "utils.hpp"

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <toml++/toml.h>

struct Joystick {
	// Keep both keyboard and mouse configurations in memory because:
	// 1. both union{} and std::variant are pain in the ass to use
	// 2. allows user to switch betweeen both configs without losing previous values

	struct {
		KeyCode up = 0xFF;
		KeyCode down = 0xFF;
		KeyCode left = 0xFF;
		KeyCode right = 0xFF;
		// Range: [0,1] i.e. works as a percentage
		float speed = 1.0f;
	} kbd;

	struct {
		// 
		// Lower value corresponds to higher sensitivity
		float sensitivity = 15.0f;
		// 1.0 is linear
		// < 1 makes center more sensitive
		float nonLinear = 1.0f;
		// Range: [0,1]
		float deadzone = 0.0f;
		bool invertXAxis = false;
		bool invertYAxis = false;
	} mouse;

	// If true, both axis will be generated from mouse movements (specifically the mouse specified by XiGamepad.srcMouse)
	bool useMouse = false;
};

struct UserProfile {
	KeyCode a = 0xFF;
	KeyCode b = 0xFF;
	KeyCode x = 0xFF;
	KeyCode y = 0xFF;

	KeyCode lb = 0xFF;
	KeyCode rb = 0xFF;

	KeyCode lt = 0xFF;
	KeyCode rt = 0xFF;

	KeyCode start = 0xFF;
	KeyCode back = 0xFF;

	KeyCode dpadUp = 0xFF;
	KeyCode dpadDown = 0xFF;
	KeyCode dpadLeft = 0xFF;
	KeyCode dpadRight = 0xFF;

	KeyCode lstickBtn = 0xFF;
	KeyCode rstickBtn = 0xFF;

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

toml::table StringifyConfig(const Config&) noexcept;
Config LoadConfig(const toml::table&) noexcept;
