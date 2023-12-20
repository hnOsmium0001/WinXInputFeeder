#include "gamepad.hpp"

XINPUT_GAMEPAD XiGamepad::ComputeXInputGamepad() const noexcept {
    XINPUT_GAMEPAD res = {};

    if (a) res.wButtons |= XINPUT_GAMEPAD_A;
    if (b) res.wButtons |= XINPUT_GAMEPAD_B;
    if (x) res.wButtons |= XINPUT_GAMEPAD_X;
    if (y) res.wButtons |= XINPUT_GAMEPAD_Y;

    if (lb) res.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;
    if (rb) res.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;

    res.bLeftTrigger = lt ? 255 : 0;
    res.bRightTrigger = rt ? 255 : 0;

    if (start) res.wButtons |= XINPUT_GAMEPAD_START;
    if (back) res.wButtons |= XINPUT_GAMEPAD_BACK;

    if (dpadUp) res.wButtons |= XINPUT_GAMEPAD_DPAD_UP;
    if (dpadDown) res.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;
    if (dpadLeft) res.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;
    if (dpadRight) res.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;

    if (lstickBtn) res.wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;
    if (rstickBtn) res.wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;

    res.sThumbLX = lstickX;
    res.sThumbLY = lstickY;
    res.sThumbRX = rstickX;
    res.sThumbRY = rstickY;

    return res;
}


SRWLOCK gXiGamepadsLock = SRWLOCK_INIT;
bool gXiGamepadsEnabled[4] = {};
XiGamepad gXiGamepads[4] = {};
