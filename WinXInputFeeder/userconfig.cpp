#include "pch.hpp"

#include "userconfig.hpp"

#include "gamepad.hpp"
#include "utils.hpp"

#include <algorithm>
#include <fstream>

using namespace std::literals;

static void WriteJoystick(toml::table& profile, const Joystick& js, const KeyCode& jsBtn) {
	if (jsBtn != 0xFF) profile.emplace("Button", KeyCodeToString(jsBtn));

	if (js.useMouse) {
		profile.emplace("Type", "mouse"s);
		profile.emplace("Sensitivity", js.mouse.sensitivity);
		profile.emplace("NonLinearSensitivity", js.mouse.nonLinear);
		profile.emplace("Deadzone", js.mouse.deadzone);
		profile.emplace("InvertXAxis", js.mouse.invertXAxis);
		profile.emplace("InvertYAxis", js.mouse.invertYAxis);
	}
	else {
		profile.emplace("Type", "keyboard"s);
		if (js.kbd.up != 0xFF) profile.emplace("Up", KeyCodeToString(js.kbd.up));
		if (js.kbd.down != 0xFF) profile.emplace("Down", KeyCodeToString(js.kbd.down));
		if (js.kbd.left != 0xFF) profile.emplace("Left", KeyCodeToString(js.kbd.left));
		if (js.kbd.right != 0xFF) profile.emplace("Right", KeyCodeToString(js.kbd.right));
		if (js.kbd.speed != 1.0f) profile.emplace("Speed", js.kbd.speed);
	}
}

toml::table SaveConfig(const Config& config) noexcept {
	toml::table res;

	toml::table general;
	general.emplace("MouseCheckFrequency", config.mouseCheckFrequency);
	res.emplace("General", general);

	toml::table hotkeys;
	hotkeys.emplace("ShowUI", KeyCodeToString(config.hotkeyShowUI));
	hotkeys.emplace("CaptureCursor", KeyCodeToString(config.hotkeyCaptureCursor));
	res.emplace("HotKeys", hotkeys);

	toml::table profiles;
	for (auto&& [vName, vProfile] : config.profiles) {
		toml::table profile;
		if (vProfile.a != 0xFF) profile.emplace("A", KeyCodeToString(vProfile.a));
		if (vProfile.b != 0xFF) profile.emplace("B", KeyCodeToString(vProfile.b));
		if (vProfile.x != 0xFF) profile.emplace("X", KeyCodeToString(vProfile.x));
		if (vProfile.y != 0xFF) profile.emplace("Y", KeyCodeToString(vProfile.y));
		if (vProfile.lb != 0xFF) profile.emplace("LB", KeyCodeToString(vProfile.lb));
		if (vProfile.rb != 0xFF) profile.emplace("RB", KeyCodeToString(vProfile.rb));
		if (vProfile.lt != 0xFF) profile.emplace("LT", KeyCodeToString(vProfile.lt));
		if (vProfile.rt != 0xFF) profile.emplace("RT", KeyCodeToString(vProfile.rt));
		if (vProfile.start != 0xFF) profile.emplace("Start", KeyCodeToString(vProfile.start));
		if (vProfile.back != 0xFF) profile.emplace("Back", KeyCodeToString(vProfile.back));
		if (vProfile.dpadUp != 0xFF) profile.emplace("DpadUp", KeyCodeToString(vProfile.dpadUp));
		if (vProfile.dpadDown != 0xFF) profile.emplace("DpadDown", KeyCodeToString(vProfile.dpadDown));
		if (vProfile.dpadLeft != 0xFF) profile.emplace("DpadLeft", KeyCodeToString(vProfile.dpadLeft));
		if (vProfile.dpadRight != 0xFF) profile.emplace("DpadRight", KeyCodeToString(vProfile.dpadRight));

		toml::table lstick;
		WriteJoystick(profile, vProfile.lstick, vProfile.rstickBtn);
		profile.emplace("LStick", lstick);

		toml::table rstick;
		WriteJoystick(profile, vProfile.rstick, vProfile.lstickBtn);
		profile.emplace("RStick", rstick);

		profiles.emplace(vName, profile);
	}
	res.emplace("Profiles", profiles);

	toml::array x360s;
	x360s.reserve(config.x360Count);
	for (int i = 0; i < config.x360Count; ++i) {
		auto& vX360 = config.x360s[i];

		toml::table x360;
		x360.emplace("Profile", vX360->first);
		x360s.push_back(x360);
	}
	res.emplace("X360", x360s);

	toml::array dualshocks;
	// TODO
	res.emplace("DualShock", dualshocks);

	return res;
}

static KeyCode ReadKeyCode(toml::node_view<const toml::node> t) {
	auto str = t.value<std::string_view>();
	if (str)
		return KeyCodeFromString(str.value()).value_or(0xFF);
	else
		return 0xFF;
}

static void ReadJoystick(toml::node_view<const toml::node> t, Joystick& js, KeyCode& jsBtn) {
	jsBtn = ReadKeyCode(t["Button"]);

	if (const auto& v = t["Type"];
		v == "keyboard")
	{
		js.useMouse = false;
		js.kbd.up = ReadKeyCode(t["Up"]);
		js.kbd.down = ReadKeyCode(t["Down"]);
		js.kbd.left = ReadKeyCode(t["Left"]);
		js.kbd.right = ReadKeyCode(t["Right"]);
		js.kbd.speed = std::clamp(t["Speed"].value_or<float>(1.0f), 0.0f, 1.0f);
	}
	else if (v == "mouse") {
		js.useMouse = true;
		js.mouse.sensitivity = t["Sensitivity"].value_or<float>(50.0f);
		js.mouse.nonLinear = t["NonLinearSensitivity"].value_or<float>(0.8f);
		js.mouse.deadzone = t["Deadzone"].value_or<float>(0.02f);
		js.mouse.invertXAxis = t["InvertXAxis"].value_or<bool>(false);
		js.mouse.invertYAxis = t["InvertYAxis"].value_or<bool>(false);
	}
	else {
		// Resets to inactive mode
		js = {};
	}
}

Config LoadConfig(const toml::table& toml) noexcept {
	Config config;

	config.mouseCheckFrequency = toml["General"]["MouseCheckFrequency"].value_or<int>(75);
	config.hotkeyShowUI = ReadKeyCode(toml["HotKeys"]["ShowUI"]);
	config.hotkeyCaptureCursor = ReadKeyCode(toml["HotKeys"]["CaptureCursor"]);

	if (auto profiles = toml["UserProfiles"].as_table()) {
		for (auto&& [key, val] : *profiles) {
			auto e1 = val.as_table();
			if (!e1) continue;
			auto& tomlProfile = *e1;
			auto name = key.str();

			UserProfile profile;
			profile.a = ReadKeyCode(tomlProfile["A"]);
			profile.b = ReadKeyCode(tomlProfile["B"]);
			profile.x = ReadKeyCode(tomlProfile["X"]);
			profile.y = ReadKeyCode(tomlProfile["Y"]);
			profile.lb = ReadKeyCode(tomlProfile["LB"]);
			profile.rb = ReadKeyCode(tomlProfile["RB"]);
			profile.lt = ReadKeyCode(tomlProfile["LT"]);
			profile.rt = ReadKeyCode(tomlProfile["RT"]);
			profile.start = ReadKeyCode(tomlProfile["Start"]);
			profile.back = ReadKeyCode(tomlProfile["Back"]);
			profile.dpadUp = ReadKeyCode(tomlProfile["DpadUp"]);
			profile.dpadDown = ReadKeyCode(tomlProfile["DpadDown"]);
			profile.dpadLeft = ReadKeyCode(tomlProfile["DpadLeft"]);
			profile.dpadRight = ReadKeyCode(tomlProfile["DpadRight"]);
			ReadJoystick(tomlProfile["LStick"], profile.lstick, profile.rstickBtn);
			ReadJoystick(tomlProfile["RStick"], profile.rstick, profile.lstickBtn);

			auto [DISCARD, success] = config.profiles.try_emplace(std::string(name), std::move(profile));
			if (!success) {
				LOG_DEBUG(L"User profile '{}' already exists, cannot add", Utf8ToWide(name));
			}
		}
	}

	if (auto x360binds = toml["X360"].as_array()) {
		for (auto&& entry : *x360binds) {
			if (config.x360Count >= 4) {
				LOG_DEBUG(L"Too many X360 gamepads, skipping the rest");
				break;
			}

			auto etableOpt = entry.as_table();
			if (!etableOpt) continue;
			auto etable = *etableOpt;

			auto profileNameOpt = etable["Profile"].value<std::string_view>();
			if (!profileNameOpt) continue;
			auto profileName = *profileNameOpt;

			auto iter = config.profiles.find(profileName);
			if (iter == config.profiles.end()) {
				LOG_DEBUG(L"Cannout find profile '{}' for binding X360 gamepads, skipping", Utf8ToWide(profileName));
				continue;
			}

			config.x360s[config.x360Count++] = &*iter;
		}
	}

	auto dualshockBinds = toml["DualShock"];
	// TODO

	return config;
}
