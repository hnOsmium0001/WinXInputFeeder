#pragma once

#include <optional>
#include <string>
#include <string_view>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// Win32 Vkey keycode
using KeyCode = BYTE;

void InitKeyCodeConv();
std::string_view KeyCodeToString(KeyCode key);
std::optional<KeyCode> KeyCodeFromString(std::string_view str);

bool IsKeyCodeMouseButton(KeyCode key);

GUID ParseRawInputDeviceGUID(std::wstring_view name);

// For RIM_TYPExxx values
std::wstring_view RawInputTypeToString(DWORD type);

struct IdevDevice {
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    // TODO the GUID doesn't change for different devices at all, need a different way to identifying the device
    //      some part of the string (particularly the last xxxx between & and #) seems to check every time the same device is reconnected, so we can't just use the whole string
    // We might need to parse more parts of the name
    GUID guid = {};
    std::wstring nameWide;
    std::string nameUtf8;
    RID_DEVICE_INFO info;

    static IdevDevice FromHANDLE(HANDLE hDevice);
};

// All pointer params are optional
void PollInputDevices(std::vector<IdevDevice>& out);
