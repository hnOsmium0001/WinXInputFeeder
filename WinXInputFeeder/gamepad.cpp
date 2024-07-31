#include "pch.hpp"

#include "gamepad.hpp"

void X360Gamepad::SetButton(XUSB_BUTTON btn, bool onoff) noexcept {
	switch (btn) {
#define CASE(e) case e: if (onoff) state.wButtons |= e; else state.wButtons &= ~e; break
		CASE(XUSB_GAMEPAD_DPAD_UP);
		CASE(XUSB_GAMEPAD_DPAD_DOWN);
		CASE(XUSB_GAMEPAD_DPAD_LEFT);
		CASE(XUSB_GAMEPAD_DPAD_RIGHT);
		CASE(XUSB_GAMEPAD_START);
		CASE(XUSB_GAMEPAD_BACK);
		CASE(XUSB_GAMEPAD_LEFT_THUMB);
		CASE(XUSB_GAMEPAD_RIGHT_THUMB);
		CASE(XUSB_GAMEPAD_LEFT_SHOULDER);
		CASE(XUSB_GAMEPAD_RIGHT_SHOULDER);
		CASE(XUSB_GAMEPAD_GUIDE);
		CASE(XUSB_GAMEPAD_A);
		CASE(XUSB_GAMEPAD_B);
		CASE(XUSB_GAMEPAD_X);
		CASE(XUSB_GAMEPAD_Y);
	}
}

SRWLOCK gX360GamepadsLock = SRWLOCK_INIT;
bool gX360GamepadsEnabled[4] = {};
X360Gamepad gX360Gamepads[4] = {};
