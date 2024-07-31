#include "pch.hpp"

#include <format>
#include <stdexcept>
#include <utility>

#include "gamepad.hpp"

ViGEm::ViGEm()
	: hvigem{ vigem_alloc() }
{
	VIGEM_ERROR err;

	err = vigem_connect(hvigem);
	if (!VIGEM_SUCCESS(err))
		throw std::runtime_error(std::format("Failed to connect ViGEm bus"));
}

ViGEm::~ViGEm() {
	vigem_disconnect(hvigem);
	vigem_free(hvigem);
}

ViGEm::ViGEm(ViGEm&& that)
	: hvigem{ std::exchange(that.hvigem, nullptr) } {}

ViGEm& ViGEm::operator=(ViGEm&& that) {
	vigem_disconnect(hvigem);
	vigem_free(hvigem);
	hvigem = std::exchange(that.hvigem, nullptr);
	return *this;
}

X360Gamepad::X360Gamepad(const ViGEm& client)
	: hvigem{ client.hvigem }
	, htarget{ vigem_target_x360_alloc() }
{
	VIGEM_ERROR err;

	err = vigem_target_add(client.hvigem, htarget);
	if (!VIGEM_SUCCESS(err))
		throw std::runtime_error(std::format("Failed to add X360 gamepad to ViGEm bus"));
}

X360Gamepad::~X360Gamepad() {
	vigem_target_remove(hvigem, htarget);
	vigem_target_free(htarget);
}

X360Gamepad::X360Gamepad(X360Gamepad&& that)
	: hvigem{ std::exchange(that.hvigem, nullptr) }
	, htarget{ std::exchange(that.htarget, nullptr) } {}

X360Gamepad& X360Gamepad::operator=(X360Gamepad&& that) {
	vigem_target_remove(hvigem, htarget);
	vigem_target_free(htarget);
	hvigem = std::exchange(that.hvigem, nullptr);
	htarget = std::exchange(that.htarget, nullptr);
	return *this;
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
