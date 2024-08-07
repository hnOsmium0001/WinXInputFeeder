#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <minwindef.h>

// Win32 Vkey keycode
using KeyCode = BYTE;

void InitKeyCodeConv();
std::string_view KeyCodeToString(KeyCode key) noexcept;
std::optional<KeyCode> KeyCodeFromString(std::string_view str) noexcept;

bool IsKeyCodeMouseButton(KeyCode key) noexcept;

// For RIM_TYPExxx values
std::wstring_view RawInputTypeToString(DWORD type);

enum class IdevKind {
    Keyboard = RIM_TYPEMOUSE,
    Mouse = RIM_TYPEKEYBOARD,
    Hid = RIM_TYPEHID,
};

struct IdevDevice {
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    std::string nameUtf8;
    RID_DEVICE_INFO info;
    USHORT vendorId;
    USHORT productId;

    IdevKind GetKind() const noexcept { return static_cast<IdevKind>(info.dwType); }

    static IdevDevice FromHANDLE(HANDLE hDevice);
};
