#include "pch.hpp"

#include "userconfig.hpp"

#include "gamepad.hpp"
#include "utils.hpp"

#include <algorithm>
#include <fstream>

using namespace std::literals;

static void WriteJoystick(toml::table& profile, const Joystick& js) {
	profile.emplace("Type", js.useMouse ? "mouse"s : "keyboard"s);
	profile.emplace("Speed", js.speed);
	profile.emplace("Sensitivity", js.sensitivity);
	profile.emplace("NonLinearSensitivity", js.nonLinear);
	profile.emplace("Deadzone", js.deadzone);
	profile.emplace("InvertXAxis", js.invertXAxis);
	profile.emplace("InvertYAxis", js.invertYAxis);
}

toml::table SaveConfig(const Config& vConfig) {
	toml::table res;

	toml::table general;
	general.emplace("MouseCheckFrequency", vConfig.mouseCheckFrequency);
	res.emplace("General", general);

	toml::table hotkeys;
	hotkeys.emplace("ShowUI", KeyCodeToString(vConfig.hotkeyShowUI));
	hotkeys.emplace("CaptureCursor", KeyCodeToString(vConfig.hotkeyCaptureCursor));
	res.emplace("HotKeys", hotkeys);

	toml::table profiles;
	for (auto&& [vName, vProfile] : vConfig.profiles) {
		toml::table profile;

		for (int vBtn = 0; vBtn < kX360ButtonCount; ++vBtn) {
			KeyCode vKey = vProfile.buttons[vBtn];
			if (vBtn != 0xFF)
				profile.emplace(X360ButtonToString(static_cast<X360Button>(vBtn)), KeyCodeToString(vKey));
		}

		toml::table lstick;
		WriteJoystick(profile, vProfile.lstick);
		profile.emplace("LStick", lstick);

		toml::table rstick;
		WriteJoystick(profile, vProfile.rstick);
		profile.emplace("RStick", rstick);

		profiles.emplace(vName, profile);
	}
	res.emplace("Profiles", profiles);

	toml::array x360s;
	x360s.reserve(vConfig.x360Count);
	for (int i = 0; i < vConfig.x360Count; ++i) {
		auto& vX360 = vConfig.x360s[i];

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

static void ReadJoystick(toml::node_view<const toml::node> t, Joystick& js) {
	if (const auto& v = t["Type"];
		v == "keyboard")
		js.useMouse = false;
	else if (v == "mouse")
		js.useMouse = true;

	js.speed = std::clamp(t["Speed"].value_or<float>(1.0f), 0.0f, 1.0f);
	js.sensitivity = t["Sensitivity"].value_or<float>(50.0f);
	js.nonLinear = t["NonLinearSensitivity"].value_or<float>(0.8f);
	js.deadzone = t["Deadzone"].value_or<float>(0.02f);
	js.invertXAxis = t["InvertXAxis"].value_or<bool>(false);
	js.invertYAxis = t["InvertYAxis"].value_or<bool>(false);
}

Config LoadConfig(const toml::table& fConfig) {
	Config config;

	auto fGeneral = fConfig["General"];
	config.mouseCheckFrequency = fGeneral["MouseCheckFrequency"].value_or<int>(75);

	auto fHotkey = fConfig["HotKeys"];
	config.hotkeyShowUI = ReadKeyCode(fHotkey["ShowUI"]);
	config.hotkeyCaptureCursor = ReadKeyCode(fHotkey["CaptureCursor"]);

	if (auto fProfiles = fConfig["UserProfiles"].as_table()) {
		for (auto&& [key, val] : *fProfiles) {
			auto e1 = val.as_table();
			if (!e1) continue;
			auto& fProfile = *e1;
			auto fName = key.str();

			UserProfile profile;
			for (int fBtn = 0; fBtn < kX360ButtonCount; ++fBtn) {
				KeyCode fKey = ReadKeyCode(fProfile[X360ButtonToString(static_cast<X360Button>(fBtn))]);
				profile.buttons[fBtn] = fKey;
			}
			ReadJoystick(fProfile["LStick"], profile.lstick);
			ReadJoystick(fProfile["RStick"], profile.rstick);

			auto [DISCARD, success] = config.profiles.try_emplace(std::string(fName), std::move(profile));
			if (!success) {
				LOG_DEBUG(L"User profile '{}' already exists, cannot add", Utf8ToWide(fName));
			}
		}
	}

	if (auto fX360binds = fConfig["X360"].as_array()) {
		for (auto&& fEntry : *fX360binds) {
			if (config.x360Count >= 4) {
				LOG_DEBUG(L"Too many X360 gamepads, skipping the rest");
				break;
			}

			auto fTableOpt = fEntry.as_table();
			if (!fTableOpt) continue;
			auto fTable = *fTableOpt;

			auto fProfileNameOpt = fTable["Profile"].value<std::string_view>();
			if (!fProfileNameOpt) continue;
			auto fProfileName = *fProfileNameOpt;

			auto iter = config.profiles.find(fProfileName);
			if (iter == config.profiles.end()) {
				LOG_DEBUG(L"Cannout find profile '{}' for binding X360 gamepads, skipping", Utf8ToWide(fProfileName));
				continue;
			}

			config.x360s[config.x360Count++] = &*iter;
		}
	}

	auto fDualshockBinds = fConfig["DualShock"];
	// TODO

	return config;
}
