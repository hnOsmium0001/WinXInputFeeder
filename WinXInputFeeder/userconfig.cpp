#include "pch.hpp"

#include "userconfig.hpp"

#include "gamepad.hpp"

#include <algorithm>
#include <fstream>

using namespace std::literals;

Config gConfig;
ConfigEvents gConfigEvents;

void ReloadConfigFromDesignatedPath() {
    // Load config from the designated config file
    WCHAR buf[MAX_PATH];
    DWORD numChars = GetModuleFileNameW(nullptr, buf, MAX_PATH); // Returns .exe file path
    auto configPath = std::filesystem::path(buf, buf + numChars).remove_filename() / L"WinXInputFeeder.toml";

    LOG_DEBUG(L"Designated config path: {}", configPath.native());

    ReloadConfig(configPath);
}

void ReloadConfig(const std::filesystem::path& path) {
    gConfig = LoadConfig(toml::parse_file(path));
    
    gConfigEvents.onMouseCheckFrequencyChanged(gConfig.mouseCheckFrequency);

    for (int userIndex = 0; userIndex < 4; ++userIndex) {
        const auto& profileName = gConfig.xiGamepadBindings[userIndex];
        if (profileName.empty()) continue;

        auto iter = gConfig.profiles.find(profileName);
        if (iter != gConfig.profiles.end()) {
            const auto& profile = iter->second;
            LOG_DEBUG(L"Binding profile '{}' to gamepad {}", Utf8ToWide(profileName), userIndex);
            SrwExclusiveLock lock(gX360GamepadsLock);
            gX360GamepadsEnabled[userIndex] = true;
            gX360Gamepads[userIndex] = {};
            gX360Gamepads[userIndex].profile = &profile;
            gConfigEvents.onGamepadBindingChanged(userIndex, profileName, profile);
        }
        else {
            LOG_DEBUG(L"Cannout find profile '{}' for binding gamepads, skipping", Utf8ToWide(profileName));
        }
    }
}

toml::table StringifyConfig(const Config& config)  noexcept {
    return {}; // TODO
}

static void ReadButton(toml::node_view<const toml::node> t, Button& btn) {
    btn.keyCode = KeyCodeFromString(t.value_or<std::string_view>(""sv)).value_or(0xFF);
}

static void ReadJoystick(toml::node_view<const toml::node> t, Joystick& js, Button& jsBtn) {
    ReadButton(t["Button"], jsBtn);

    if (const auto& v = t["Type"];
        v == "keyboard")
    {
        js.useMouse = false;
        ReadButton(t["Up"], js.kbd.up);
        ReadButton(t["Down"], js.kbd.down);
        ReadButton(t["Left"], js.kbd.left);
        ReadButton(t["Right"], js.kbd.right);
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

    // This should map nothing, effectively hiding this gamepad slot
    config.profiles.emplace("NULL"sv, UserProfile{});

    config.mouseCheckFrequency = toml["General"]["MouseCheckFrequency"].value_or<int>(75);
    config.hotkeyShowUI = KeyCodeFromString(toml["HotKeys"]["ShowUI"].value_or<std::string_view>(""sv)).value_or(0xFF);
    config.hotkeyCaptureCursor = KeyCodeFromString(toml["HotKeys"]["CaptureCursor"].value_or<std::string_view>(""sv)).value_or(0xFF);

    if (auto tomlProfiles = toml["UserProfiles"].as_table()) {
        for (auto&& [key, val] : *tomlProfiles) {
            auto e1 = val.as_table();
            if (!e1) continue;
            auto& tomlProfile = *e1;
            auto name = key.str();

            UserProfile profile;
            ReadButton(tomlProfile["A"], profile.a);
            ReadButton(tomlProfile["B"], profile.b);
            ReadButton(tomlProfile["X"], profile.x);
            ReadButton(tomlProfile["Y"], profile.y);
            ReadButton(tomlProfile["LB"], profile.lb);
            ReadButton(tomlProfile["RB"], profile.rb);
            ReadButton(tomlProfile["LT"], profile.lt);
            ReadButton(tomlProfile["RT"], profile.rt);
            ReadButton(tomlProfile["Start"], profile.start);
            ReadButton(tomlProfile["Back"], profile.back);
            ReadButton(tomlProfile["DpadUp"], profile.dpadUp);
            ReadButton(tomlProfile["DpadDown"], profile.dpadDown);
            ReadButton(tomlProfile["DpadLeft"], profile.dpadLeft);
            ReadButton(tomlProfile["DpadRight"], profile.dpadRight);
            ReadJoystick(tomlProfile["LStick"], profile.lstick, profile.rstickBtn);
            ReadJoystick(tomlProfile["RStick"], profile.rstick, profile.lstickBtn);

            auto [DISCARD, success] = config.profiles.try_emplace(std::string(name), std::move(profile));
            if (!success) {
                LOG_DEBUG(L"User profile '{}' already exists, cannot add", Utf8ToWide(name));
            }
        }
    }

    for (int i = 0; i < 4; ++i) {
        auto key = std::format("Gamepad{}", i);
        config.xiGamepadBindings[i] = toml["Binding"][key].value_or<std::string>(""s);
    }

    return config;
}

void BindProfileToGamepad(int userIndex, const UserProfile& profile) {
    SrwExclusiveLock lock(gX360GamepadsLock);
    gX360GamepadsEnabled[userIndex] = true;
    gX360Gamepads[userIndex] = {};
    gX360Gamepads[userIndex].profile = &profile;
}
