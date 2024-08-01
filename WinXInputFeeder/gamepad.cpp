#include "pch.hpp"

#include <format>
#include <stdexcept>
#include <utility>

#include "gamepad.hpp"

using namespace std::literals;

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

ViGEm::ViGEm(ViGEm&& that) noexcept
	: hvigem{ std::exchange(that.hvigem, nullptr) } {}

ViGEm& ViGEm::operator=(ViGEm&& that) noexcept {
	vigem_disconnect(hvigem);
	vigem_free(hvigem);
	hvigem = std::exchange(that.hvigem, nullptr);
	return *this;
}

X360Button X360ButtonFromViGEm(XUSB_BUTTON btn) noexcept {
	unsigned long idx;
	unsigned char res = _BitScanForward(&idx, btn);
	return res == 0 ? static_cast<X360Button>(idx) : X360Button::None;
}

XUSB_BUTTON X360ButtonToViGEm(X360Button btn) noexcept {
	assert(IsX360ButtonDirectMap(btn));
	return static_cast<XUSB_BUTTON>(1 << std::to_underlying(btn));
}

constexpr std::string_view kX360ButtonStrings[] = {
	// Direct mapping
	"DPadUp"sv,
	"DPadDown"sv,
	"DPadLeft"sv,
	"DPadRight"sv,
	"Start"sv,
	"Back"sv,
	"LStick"sv,
	"RStick"sv,
	"LB"sv,
	"RB"sv,
	"Guide"sv,
	"A"sv,
	"B"sv,
	"X"sv,
	"Y"sv,

	// Analog counterpart
	"LT"sv,
	"RT"sv,
	"LStickUp"sv, "LStickDown"sv, "LStickLeft"sv, "LStickRight"sv,
	"RStickUp"sv, "RStickDown"sv, "RStickLeft"sv, "RStickRight"sv,
};

std::string_view X360ButtonToString(X360Button btn) noexcept {
	return kX360ButtonStrings[std::to_underlying(btn)];
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

X360Gamepad::X360Gamepad(X360Gamepad&& that) noexcept
	: hvigem{ std::exchange(that.hvigem, nullptr) }
	, htarget{ std::exchange(that.htarget, nullptr) } {}

X360Gamepad& X360Gamepad::operator=(X360Gamepad&& that) noexcept {
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
