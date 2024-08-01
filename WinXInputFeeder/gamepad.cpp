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

bool X360Gamepad::GetButton(XUSB_BUTTON btn) const noexcept {
	// When an integral value is coerced into bool, all non-zero values are turned to 1 (and zero to 0)
	return state.wButtons & btn;
}

void X360Gamepad::SetButton(XUSB_BUTTON btn, bool onoff) noexcept {
	if (onoff) 
		state.wButtons |= btn;
	else 
		state.wButtons &= ~btn;
}

void X360Gamepad::SendReport() {
	vigem_target_x360_update(hvigem, htarget, state);
}
