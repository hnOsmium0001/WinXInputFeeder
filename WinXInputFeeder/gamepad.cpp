#include "pch.hpp"

#include <format>
#include <stdexcept>

#include "gamepad.hpp"

ViGEm::ViGEm()
	: hvigem{ vigem_alloc() }
{
	VIGEM_ERROR err;

	err = vigem_connect(hvigem);
	if (err != VIGEM_ERROR_NONE)
		throw std::runtime_error(std::format("Failed to connect ViGEm bus"));
}

ViGEm::~ViGEm() {
	vigem_disconnect(hvigem);
	vigem_free(hvigem);
}

X360Gamepad::X360Gamepad(const ViGEm& client)
	: hvigem{ client.hvigem }
	, htarget{ vigem_target_x360_alloc() }
{
	vigem_target_add(client.hvigem, htarget);
}

X360Gamepad::~X360Gamepad() {
	vigem_target_remove(hvigem, htarget);
	vigem_target_free(htarget);
}

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

void X360Gamepad::SendReport() {
	vigem_target_x360_update(hvigem, htarget, state);
}
