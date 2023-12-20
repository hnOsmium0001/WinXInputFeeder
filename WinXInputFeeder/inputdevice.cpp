#include "inputdevice.hpp"

#include <cassert>
#include <initializer_list>
#include <unordered_map>

#include <Rpc.h>

using namespace std::literals;

static std::unordered_map<std::string_view, KeyCode> gStr2Keycode;
static std::string_view gKeycode2Str[0xFF];

void InitKeyCodeConv() {
    auto add = [](KeyCode keycode, std::initializer_list<std::string_view> names) -> void {
        assert(names.size() >= 1);
        for (const auto& name : names)
            gStr2Keycode.emplace(name, keycode);
        gKeycode2Str[keycode] = *names.begin();
    };

    // https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes

    add(VK_LBUTTON, { "MouseLeft"sv });
    add(VK_RBUTTON, { "MouseRight"sv });
    /*add(VK_CANCEL, {"CtrlPause"sv);}*/
    add(VK_MBUTTON, { "MouseMiddle"sv });
    add(VK_XBUTTON1, { "MouseX1"sv });
    add(VK_XBUTTON2, { "MouseX2"sv });
    // 0x07 ---- Undefined
    add(VK_BACK, { "Backspace"sv });
    add(VK_TAB, { "Tab"sv });
    // 0x0A-0B ---- Reserved
    /*add(VK_CLEAR, {"CLEAR key"sv);}*/
    add(VK_RETURN, { "Enter"sv });
    // 0x0E-0F ---- Undefined
    /* // See below, we use the individual left/right keys
    add(VK_SHIFT, {"SHIFT key"sv});
    add(VK_CONTROL, {"CTRL key"sv});
    add(VK_MENU, {"ALT key"sv});
    */
    /*add(VK_PAUSE, {"PAUSE key"sv);}*/
    /*add(VK_CAPITAL, {"CAPS LOCK key"sv);}*/
    /*
    add(VK_KANA, {"IME Kana mode"sv});
    add(VK_HANGUL, {"IME Hangul mode"sv});
    add(VK_IME_ON, {"IME On"sv});
    add(VK_JUNJA, {"IME Junja mode"sv});
    add(VK_FINAL, {"IME final mode"sv});
    add(VK_HANJA, {"IME Hanja mode"sv});
    add(VK_KANJI, {"IME Kanji mode"sv});
    add(VK_IME_OFF, {"IME Off"sv});
    */
    /*add(VK_ESCAPE, {"ESC key"sv);}*/
    /*
    add(VK_CONVERT, {"IME convert"sv});
    add(VK_NONCONVERT, {"IME nonconvert"sv});
    add(VK_ACCEPT, {"IME accept"sv});
    add(VK_MODECHANGE, {"IME mode change request"sv});
    */
    add(VK_SPACE, { "Space"sv });
    add(VK_PRIOR, { "PageUp"sv });
    add(VK_NEXT, { "PageDown"sv });
    add(VK_END, { "End"sv });
    add(VK_HOME, { "Home"sv });
    add(VK_LEFT, { "LeftArrow"sv });
    add(VK_UP, { "UpArrow"sv });
    add(VK_RIGHT, { "RightArrow"sv });
    add(VK_DOWN, { "DownArrow"sv });
    /*add(VK_SELECT, {"SELECT key"sv);}*/
    /*add(VK_PRINT, {"PRINT key"sv);}*/
    /*add(VK_EXECUTE, {"EXECUTE key"sv);}*/
    /*add(VK_SNAPSHOT, {"PRINT SCREEN key"sv);}*/
    add(VK_INSERT, { "Insert"sv });
    add(VK_DELETE, { "Delete"sv });
    /*add(VK_HELP, {"HELP key"sv);}*/
    add('0', { "0"sv });
    add('1', { "1"sv });
    add('2', { "2"sv });
    add('3', { "3"sv });
    add('4', { "4"sv });
    add('5', { "5"sv });
    add('6', { "6"sv });
    add('7', { "7"sv });
    add('8', { "8"sv });
    add('9', { "9"sv });
    // 0x3A-40 ---- Undefined
    add('A', { "A"sv });
    add('B', { "B"sv });
    add('C', { "C"sv });
    add('D', { "D"sv });
    add('E', { "E"sv });
    add('F', { "F"sv });
    add('G', { "G"sv });
    add('H', { "H"sv });
    add('I', { "I"sv });
    add('J', { "J"sv });
    add('K', { "K"sv });
    add('L', { "L"sv });
    add('M', { "M"sv });
    add('N', { "N"sv });
    add('O', { "O"sv });
    add('P', { "P"sv });
    add('Q', { "Q"sv });
    add('R', { "R"sv });
    add('S', { "S"sv });
    add('T', { "T"sv });
    add('U', { "U"sv });
    add('V', { "V"sv });
    add('W', { "W"sv });
    add('X', { "X"sv });
    add('Y', { "Y"sv });
    add('Z', { "Z"sv });
    add(VK_LWIN, { "LWin"sv });
    add(VK_RWIN, { "RWin"sv });
    add(VK_APPS, { "Apps"sv });
    // 0x5E ---- Reserved
    /*add(VK_SLEEP, {"Computer Sleep key"sv);}*/
    add(VK_NUMPAD0, { "Numpad0"sv });
    add(VK_NUMPAD1, { "Numpad1"sv });
    add(VK_NUMPAD2, { "Numpad2"sv });
    add(VK_NUMPAD3, { "Numpad3"sv });
    add(VK_NUMPAD4, { "Numpad4"sv });
    add(VK_NUMPAD5, { "Numpad5"sv });
    add(VK_NUMPAD6, { "Numpad6"sv });
    add(VK_NUMPAD7, { "Numpad7"sv });
    add(VK_NUMPAD8, { "Numpad8"sv });
    add(VK_NUMPAD9, { "Numpad9"sv });
    add(VK_MULTIPLY, { "NumpadMultiply"sv });
    add(VK_ADD, { "NumpadAdd"sv });
    add(VK_SEPARATOR, { "Separator"sv }); // I don't think this key exists on modern keyboards
    add(VK_SUBTRACT, { "NumpadSubtract"sv });
    add(VK_DECIMAL, { "NumpadDecimal"sv });
    add(VK_DIVIDE, { "NumpadDivide"sv });
    add(VK_F1, { "F1"sv });
    add(VK_F2, { "F2"sv });
    add(VK_F3, { "F3"sv });
    add(VK_F4, { "F4"sv });
    add(VK_F5, { "F5"sv });
    add(VK_F6, { "F6"sv });
    add(VK_F7, { "F7"sv });
    add(VK_F8, { "F8"sv });
    add(VK_F9, { "F9"sv });
    add(VK_F10, { "F10"sv });
    add(VK_F11, { "F11"sv });
    add(VK_F12, { "F12"sv });
    add(VK_F13, { "F13"sv });
    add(VK_F14, { "F14"sv });
    add(VK_F15, { "F15"sv });
    add(VK_F16, { "F16"sv });
    add(VK_F17, { "F17"sv });
    add(VK_F18, { "F18"sv });
    add(VK_F19, { "F19"sv });
    add(VK_F20, { "F20"sv });
    add(VK_F21, { "F21"sv });
    add(VK_F22, { "F22"sv });
    add(VK_F23, { "F23"sv });
    add(VK_F24, { "F24"sv });
    // 0x88-8F ---- Unassigned
    /*add(VK_NUMLOCK, {"NUM LOCK key"sv);}*/
    /*add(VK_SCROLL, {"SCROLL LOCK key"sv);}*/
    // 0x92-96 ---- OEM specific
    // 0x97-9F ---- Unassigned
    add(VK_LSHIFT, { "LShift"sv });
    add(VK_RSHIFT, { "RShift"sv });
    add(VK_LCONTROL, { "LCtrl"sv });
    add(VK_RCONTROL, { "RCtrl"sv });
    add(VK_LMENU, { "LAlt"sv });
    add(VK_RMENU, { "RAlt"sv });
    /*
    add(VK_BROWSER_BACK, {"Browser Back key"sv});
    add(VK_BROWSER_FORWARD, {"Browser Forward key"sv});
    add(VK_BROWSER_REFRESH, {"Browser Refresh key"sv});
    add(VK_BROWSER_STOP, {"Browser Stop key"sv});
    add(VK_BROWSER_SEARCH, {"Browser Search key"sv});
    add(VK_BROWSER_FAVORITES, {"Browser Favorites key"sv});
    add(VK_BROWSER_HOME, {"Browser Start and Home key"sv});
    add(VK_VOLUME_MUTE, {"Volume Mute key"sv});
    add(VK_VOLUME_DOWN, {"Volume Down key"sv});
    add(VK_VOLUME_UP, {"Volume Up key"sv});
    add(VK_MEDIA_NEXT_TRACK, {"Next Track key"sv});
    add(VK_MEDIA_PREV_TRACK, {"Previous Track key"sv});
    add(VK_MEDIA_STOP, {"Stop Media key"sv});
    add(VK_MEDIA_PLAY_PAUSE, {"Play / Pause Media key"sv});
    add(VK_LAUNCH_MAIL, {"Start Mail key"sv});
    add(VK_LAUNCH_MEDIA_SELECT, {"Select Media key"sv});
    add(VK_LAUNCH_APP1, {"Start Application 1 key"sv});
    add(VK_LAUNCH_APP2, {"Start Application 2 key"sv});
    */
    // 0xB8-B9 ---- Reserved
    add(VK_OEM_1, { "Semicolon"sv, ";"sv });
    add(VK_OEM_PLUS, { "Equals"sv, "="sv });
    add(VK_OEM_COMMA, { "Comma"sv, ","sv });
    add(VK_OEM_MINUS, { "Minus"sv, "-"sv });
    add(VK_OEM_PERIOD, { "Period"sv,"."sv });
    add(VK_OEM_2, { "ForwardSlash"sv ,"/"sv });
    add(VK_OEM_3, { "Grave"sv ,"`"sv });
    // 0xC1-D7 ---- Reserved
    // 0xD8-DA ---- Unassigned
    add(VK_OEM_4, { "LeftBracket"sv , "["sv });
    add(VK_OEM_5, { "BackSlash"sv ,"\\"sv });
    add(VK_OEM_6, { "RightBracket"sv ,"]"sv });
    add(VK_OEM_7, { "Quote"sv ,"'"sv });
    /*add(VK_OEM_8, {"Used for miscellaneous characters; it can vary by keyboard."sv);}*/
    // 0xE0 ---- Reserved
    // 0xE1 ---- OEM specific
    /*add(VK_OEM_102, {"The <> keys on the US standard keyboard, or the \\ | key on the non - US 102 - key keyboard"sv);}*/
    // 0xE3-E4 ---- OEM specific
    /*add(VK_PROCESSKEY, {"IME PROCESS key"sv);}*/
    // 0xE6 ---- OEM specific
    /*add(VK_PACKET, {"Used to pass Unicode characters as if they were keystrokes.The VK_PACKET key is the low word of a 32 - bit Virtual Key value used for non - keyboard input methods.For more information, see Remark in KEYBDINPUT, SendInput, WM_KEYDOWN, and WM_KEYUP"sv);}*/
    // 0xE8 ---- Unassigned
    // 0xE9-F5 ---- OEM specific
    /*
    add(VK_ATTN, {"Attn key"sv});
    add(VK_CRSEL, {"CrSel key"sv});
    add(VK_EXSEL, {"ExSel key"sv});
    add(VK_EREOF, {"Erase EOF key"sv});
    add(VK_PLAY, {"Play key"sv});
    add(VK_ZOOM, {"Zoom key"sv});
    add(VK_NONAME, {"Reserved"sv});
    add(VK_PA1, {"PA1 key"sv});
    add(VK_OEM_CLEAR, {"Clear key"sv});
    */

    for (auto& slot : gKeycode2Str) {
        if (slot.empty())
            slot = "<unknown>"sv;
    }
}

std::string_view KeyCodeToString(KeyCode key) {
    return gKeycode2Str[key];
}

std::optional<KeyCode> KeyCodeFromString(std::string_view str) {
    auto iter = gStr2Keycode.find(str);
    if (iter != gStr2Keycode.end())
        return iter->second;
    else
        return {};
}

bool IsKeyCodeMouseButton(KeyCode key) {
    return key == VK_LBUTTON || key == VK_RBUTTON || key == VK_MBUTTON || key == VK_XBUTTON1 || key == VK_XBUTTON2;
}

GUID ParseRawInputDeviceGUID(std::wstring_view name) {
    // RAWINPUT device name format:
    // \\?\HID#VID_xxxx&PID_xxxx&MI_xx#x&xxxxxxxx&x&xxxx#{884b96c3-56ef-11d1-bc8c-00a0c91405dd}

    // Note on nomenclature:
    // conventionally, when MS docs refers to "GUID", they mean a UUID wrapped in curly braces like {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
    // conversely when they refer to "UUID", it just means a regular UUID

    // 32 hex digits + 4 dashes
    constexpr UINT kUuidLen = 32 + 4;

    size_t uuidBegin = name.rfind(L'{') + 1;
    size_t uuidEnd = name.rfind(L'}');
    if (uuidBegin == std::wstring_view::npos) {
        LOG_DEBUG(L"Error parsing RAWINPUT device GUID: cannot find delimitors {{ or }}");
        return {};
    }
    if (uuidEnd - uuidBegin != kUuidLen) {
        LOG_DEBUG(L"Malformed GUID in RAWINPUT device (incorrect length), name: {}", name);
        return {};
    }

    // RPC_WSTR uses unsigned short instead of wchar_t/WCHAR for some reason
    unsigned short buffer[kUuidLen + 1];

    for (size_t it = uuidBegin, i = 0; it != uuidEnd; ++it)
        buffer[i++] = name[it];
    buffer[kUuidLen] = L'\0';

    GUID result;
    if (UuidFromStringW(buffer, &result) != RPC_S_OK) {
        LOG_DEBUG(L"Malformed GUID in RAWINPUT device (UuidFromStringW error), name: {}", name);
        return {};
    }

    return result;
}

std::wstring_view RawInputTypeToString(DWORD type) {
    switch (type) {
    case RIM_TYPEKEYBOARD: return L"keyboard"sv;
    case RIM_TYPEMOUSE: return L"mouse"sv;
    }
    return L"<unknown>"sv;
}

IdevDevice IdevDevice::FromHANDLE(HANDLE hDevice) {
    IdevDevice res;

    res.hDevice = hDevice;

    res.info = {};
    res.info.cbSize = sizeof(res.info);
    UINT deviceInfoBytes = sizeof(res.info);
    GetRawInputDeviceInfoW(hDevice, RIDI_DEVICEINFO, &res.info, &deviceInfoBytes);

    UINT deviceNameLen = 0;
    GetRawInputDeviceInfoW(hDevice, RIDI_DEVICENAME, NULL, &deviceNameLen);
    res.nameWide.resize_and_overwrite(
        deviceNameLen,
        [&](wchar_t* buf, size_t) { GetRawInputDeviceInfoW(hDevice, RIDI_DEVICENAME, buf, &deviceNameLen); return deviceNameLen; });
    res.nameUtf8 = WideToUtf8(res.nameWide);

    res.guid = ParseRawInputDeviceGUID(res.nameWide);

    return res;
}

void PollInputDevices(std::vector<IdevDevice>& out) {
    UINT numDevices = 0;
    RAWINPUTDEVICELIST* devices = nullptr;
    DEFER{ free(devices); };
    if (GetRawInputDeviceList(nullptr, &numDevices, sizeof(RAWINPUTDEVICELIST)) != 0)
        return;

    if (numDevices == 0)
        return;

    UINT numDevicesSuccessfullyFetched;
    do {
        auto newPtr = (RAWINPUTDEVICELIST*)realloc(devices, sizeof(RAWINPUTDEVICELIST) * numDevices);
        if (newPtr == nullptr)
            return;
        else
            devices = newPtr;

        numDevicesSuccessfullyFetched = GetRawInputDeviceList(devices, &numDevices, sizeof(RAWINPUTDEVICELIST));
    } while (numDevicesSuccessfullyFetched == (UINT)-1 && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

    if (numDevicesSuccessfullyFetched == (UINT)-1)
        return;

    for (UINT i = 0; i < numDevicesSuccessfullyFetched; i++) {
        out.push_back(IdevDevice::FromHANDLE(devices[i].hDevice));
        LOG_DEBUG("[PollInputDevices()] {} {}", devices[i].dwType, out.back().nameWide);
    }
}
