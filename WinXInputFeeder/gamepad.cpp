#include "pch.hpp"

#include "gamepad.hpp"

XUSB_REPORT XiGamepad::ComputeXInputGamepad() const noexcept {
    XUSB_REPORT res = {};

    if (a) res.wButtons |= XUSB_GAMEPAD_A;
    if (b) res.wButtons |= XUSB_GAMEPAD_B;
    if (x) res.wButtons |= XUSB_GAMEPAD_X;
    if (y) res.wButtons |= XUSB_GAMEPAD_Y;

    if (lb) res.wButtons |= XUSB_GAMEPAD_LEFT_SHOULDER;
    if (rb) res.wButtons |= XUSB_GAMEPAD_RIGHT_SHOULDER;

    res.bLeftTrigger = lt ? 255 : 0;
    res.bRightTrigger = rt ? 255 : 0;

    if (start) res.wButtons |= XUSB_GAMEPAD_START;
    if (back) res.wButtons |= XUSB_GAMEPAD_BACK;

    if (dpadUp) res.wButtons |= XUSB_GAMEPAD_DPAD_UP;
    if (dpadDown) res.wButtons |= XUSB_GAMEPAD_DPAD_DOWN;
    if (dpadLeft) res.wButtons |= XUSB_GAMEPAD_DPAD_LEFT;
    if (dpadRight) res.wButtons |= XUSB_GAMEPAD_DPAD_RIGHT;

    if (lstickBtn) res.wButtons |= XUSB_GAMEPAD_LEFT_THUMB;
    if (rstickBtn) res.wButtons |= XUSB_GAMEPAD_RIGHT_THUMB;

    res.sThumbLX = lstickX;
    res.sThumbLY = lstickY;
    res.sThumbRX = rstickX;
    res.sThumbRY = rstickY;

    return res;
}


SRWLOCK gXiGamepadsLock = SRWLOCK_INIT;
bool gXiGamepadsEnabled[4] = {};
XiGamepad gXiGamepads[4] = {};
