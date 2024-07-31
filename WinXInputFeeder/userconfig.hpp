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

struct Button {
	KeyCode keyCode = 0xFF;
};

struct Joystick {
	// Keep both keyboard and mouse configurations in memory because:
	// 1. both union{} and std::variant are pain in the ass to use
	// 2. allows user to switch betweeen both configs without losing previous values

	struct {
		Button up, down, left, right;
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
	Button a, b, x, y;
	Button lb, rb;
	Button lt, rt;
	Button start, back;
	Button dpadUp, dpadDown, dpadLeft, dpadRight;
	Button lstickBtn, rstickBtn;
	Joystick lstick, rstick;
};

struct Config {
	std::map<std::string, UserProfile, std::less<>> profiles;
	std::array<std::string, 4> xiGamepadBindings;
	// Recommends 50-100
	int mouseCheckFrequency = 75;
	KeyCode hotkeyShowUI;
	KeyCode hotkeyCaptureCursor;
};

// Container for all EventBus objects used for a given Config object
// Since Config is just a plain old object, these need to be called by code that modifies the given Config object.
struct ConfigEvents {
	EventBus<void(int)> onMouseCheckFrequencyChanged;
	EventBus<void(int userIndex, const std::string& profileName, const UserProfile& profile)> onGamepadBindingChanged;
};

extern Config gConfig;
extern ConfigEvents gConfigEvents;
void ReloadConfigFromDesignatedPath();
void ReloadConfig(const std::filesystem::path& path);

toml::table StringifyConfig(const Config&) noexcept;
Config LoadConfig(const toml::table&) noexcept;

// Lock: built-in
void BindProfileToGamepad(int userIndex, const UserProfile& profile);
