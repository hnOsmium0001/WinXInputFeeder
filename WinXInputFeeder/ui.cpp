#include "pch.hpp"

#include "ui.hpp"

#include "app.hpp"
#include "modelruntime.hpp"
#include "utils.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <string>

using namespace std::literals;

// Helper to display a little (?) mark which shows a tooltip when hovered.
// In your own code you may want to display an actual icon if you are using a merged icon fonts (see docs/FONTS.md)
static void HelpMarker(const char* desc) {
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::BeginItemTooltip()) {
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

static bool ButtonDisablable(const char* msg, bool disabled) {
	if (disabled) {
		ImGui::BeginDisabled();
		bool res = ImGui::Button(msg);
		ImGui::EndDisabled();
		return res;
	}
	else {
		return ImGui::Button(msg);
	}
}

struct UIStatePrivate {
	UIState* pub;
	FeederEngine* feeder = nullptr;
	std::string newProfileName;
	int selectedGamepadId = -1;

	UIStatePrivate(UIState& s)
		: pub{ &s }
	{
	}

	void Show();
	void ShowNavWindow();
	void ShowDetailWindow();

	void ShowButton(const X360Gamepad& gamepad, int gamepadId, X360Button btn, KeyCode boundKey);
};

UIState::UIState(App& app)
	: p{ new UIStatePrivate(*this) }
{
}

UIState::~UIState() {
	delete p;
}

void UIState::OnFeederEngine(FeederEngine* feeder) {
	auto& p = *static_cast<UIStatePrivate*>(this->p);

	p.feeder = feeder;
}

void UIState::Show() {
	auto& p = *static_cast<UIStatePrivate*>(this->p);

	p.Show();
}

void UIStatePrivate::Show() {
	if (!feeder)
		return;

	const Config& config = feeder->GetConfig();

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("WinXInputEmu")) {
			if (ImGui::MenuItem("Quit")) {
				PostQuitMessage(0);
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	ImGui::Begin("Gamepads");
	ShowNavWindow();
	ImGui::End();

	ImGui::Begin("Gamepad info");
	ShowDetailWindow();
	ImGui::End();
}

void UIStatePrivate::ShowNavWindow() {
	auto& config = feeder->GetConfig();

	auto pp = feeder->GetCurrentProfile();
	if (ImGui::Button("+profile")) {
		// Search for a new usable dummy name
		char buf[256];
		int size;
		int n = 1;
		while (true) {
			int written = snprintf(buf, sizeof(buf), "Profile %d", n);
			if (written > sizeof(buf) || n == INT_MAX) {
				size = sizeof(buf) - 1;
				break; // Bail
			}
			auto iter = config.profiles.find(std::string_view(buf, written));
			if (iter == config.profiles.end()) {
				size = written;
				break;
			}
			++n;
		} 
		newProfileName.assign(buf, size);
		ImGui::OpenPopup("Create Profile");
	}
	ImGui::SameLine();
	if (ButtonDisablable("-profile", !pp)) {
		ImGui::OpenPopup("Confirm Remove Profile");
	}

	if (ImGui::BeginPopup("Create Profile")) {
		bool invalid = newProfileName.empty();

		if (ImGui::IsWindowAppearing())
			ImGui::SetKeyboardFocusHere(); // On InputText
		if ((ImGui::InputText("Name", &newProfileName, ImGuiInputTextFlags_EnterReturnsTrue) && !invalid) ||
			ButtonDisablable("Confirm", invalid))
		{
			feeder->AddProfile(newProfileName);
			feeder->SelectProfile(&*config.profiles.find(newProfileName));
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	bool dummy = true;
	// We have to pass p_opened to make ImGui show a close button
	if (ImGui::BeginPopupModal("Confirm Remove Profile", &dummy, ImGuiWindowFlags_NoResize)) {
		ImGui::TextUnformatted("Are you sure you want to remove this profile?");
		if (ImGui::Button("Confirm")) {
			feeder->RemoveProfile(pp);
			selectedGamepadId = -1;
			pp = nullptr;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (config.profiles.empty()) {
		if (ImGui::BeginCombo("Profile", "")) {
			ImGui::MenuItem("Create a profile by clicking the +profile button above", nullptr, nullptr, false);
			ImGui::EndCombo();
		}
		return;
	}
	else {
		auto display = pp ? pp->first.c_str() : "";
		if (ImGui::BeginCombo("Profile", display)) {
			for (auto& it : config.profiles) {
				auto&& [profileName, profile] = it;
				bool selected = &it == pp;
				if (ImGui::MenuItem(profileName.c_str(), nullptr, &selected)) {
					feeder->SelectProfile(&it);
				}
			}
			ImGui::EndCombo();
		}
	}

	if (!pp)
		return;

	const std::string& profileName = pp->first;
	const ConfigProfile& profile = pp->second;
	auto x360s = feeder->GetX360s();

	if (ImGui::Button("+")) {
		feeder->AddX360();
	}
	ImGui::SameLine();
	if (ButtonDisablable("-", selectedGamepadId == -1)) {
		feeder->RemoveGamepad(selectedGamepadId);
	}

	for (int gamepadId = 0; gamepadId < x360s.size(); ++gamepadId) {
		char id[256];
		snprintf(id, sizeof(id), "Gamepad %d", gamepadId);
		bool selected = selectedGamepadId == gamepadId;
		if (ImGui::Selectable(id, &selected)) {
			selectedGamepadId = gamepadId;
		}
	}
}

void UIStatePrivate::ShowDetailWindow() {
	auto pp = feeder->GetCurrentProfile();
	if (!pp)
		return;

	const std::string& profileName = pp->first;
	const ConfigProfile& profile = pp->second;
	auto x360s = feeder->GetX360s();

	if (selectedGamepadId == -1) {
		ImGui::Text("Select a gamepad to show details");
		return;
	}

	auto& dev = x360s[selectedGamepadId];
	auto& gamepad = profile.gamepads[selectedGamepadId];

	if (ImGui::Button("Rebind##kbd")) {
		feeder->StartRebindX360Device(selectedGamepadId, IdevKind::Keyboard);
	}
	ImGui::SameLine();
	if (ImGui::Button("Unbind##kdb")) {
		feeder->RebindX360Device(selectedGamepadId, IdevKind::Keyboard, INVALID_HANDLE_VALUE);
	}
	ImGui::SameLine();
	if (dev.pendingRebindKbd) {
		ImGui::SameLine();
		ImGui::Text("press any key on the keyboard");
	}
	else if (dev.srcKbd == INVALID_HANDLE_VALUE) {
		ImGui::Text("Bound keyboard: [not bound]");
		HelpMarker("This means no key press will trigger any bound buttons, effectively disabling this gamepad from key inputs.");
	}
	else {
		ImGui::Text("Bound keyboard: %p", dev.srcKbd);
	}

	if (ImGui::Button("Rebind##mouse")) {
		feeder->StartRebindX360Device(selectedGamepadId, IdevKind::Mouse);
	}
	ImGui::SameLine();
	if (ImGui::Button("Unbind##mouse")) {
		feeder->RebindX360Device(selectedGamepadId, IdevKind::Mouse, INVALID_HANDLE_VALUE);
	}
	ImGui::SameLine();
	if (dev.pendingRebindMouse) {
		ImGui::SameLine();
		ImGui::Text("press any mouse button");
	}
	else if (dev.srcMouse == INVALID_HANDLE_VALUE) {
		ImGui::Text("Bound mouse: [not bound]");
		HelpMarker("This means no mouse button or movement will trigger any bound buttons, effectively disabling this gamepad from mouse inputs.");
	}
	else {
		ImGui::Text("Bound mouse: %p", dev.srcMouse);
	}

	// DBG
	ImGui::Text("accuMouseX: %f", dev.accuMouseX);
	ImGui::Text("accuMouseY: %f", dev.accuMouseY);
	ImGui::Text("lastAngle: %f", dev.lastAngle);

	using enum X360Button;
#define BUTTON(THE_BTN) ShowButton(dev, selectedGamepadId, THE_BTN, gamepad.buttons[static_cast<unsigned char>(THE_BTN)])
	BUTTON(LStickUp);
	ImGui::SameLine();
	BUTTON(LStickDown);
	ImGui::SameLine();
	BUTTON(LStickLeft);
	ImGui::SameLine();
	BUTTON(LStickRight);

	BUTTON(RStickUp);
	ImGui::SameLine();
	BUTTON(RStickDown);
	ImGui::SameLine();
	BUTTON(RStickLeft);
	ImGui::SameLine();
	BUTTON(RStickRight);

	BUTTON(DPadUp);
	ImGui::SameLine();
	BUTTON(DPadDown);
	ImGui::SameLine();
	BUTTON(DPadLeft);
	ImGui::SameLine();
	BUTTON(DPadRight);

	BUTTON(Start);
	ImGui::SameLine();
	BUTTON(Back);

	BUTTON(LeftThumb);
	ImGui::SameLine();
	BUTTON(RightThumb);

	BUTTON(LeftShoulder);
	ImGui::SameLine();
	BUTTON(RightShoulder);

	BUTTON(LeftTrigger);
	BUTTON(RightTrigger);

	BUTTON(Guide);

	BUTTON(A);
	ImGui::SameLine();
	BUTTON(B);
	ImGui::SameLine();
	BUTTON(X);
	ImGui::SameLine();
	BUTTON(Y);
#undef BUTTON
}

void UIStatePrivate::ShowButton(const X360Gamepad& gamepad, int gamepadId, X360Button btn, KeyCode boundKey) {
	using enum X360Button;

	const char* name = X360ButtonToString(btn).data();

	if (IsX360ButtonDirectMap(btn)) {
		bool pressed = gamepad.GetButton(X360ButtonToViGEm(btn));
		if (pressed) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
		if (ImGui::Button(name, ImVec2(80, 20)))
			feeder->StartRebindX360Mapping(gamepadId, btn);
		if (pressed) ImGui::PopStyleColor();
	}
	else {
		int value;
		float f1, f2;
		switch (btn) {
		case LeftTrigger: value = gamepad.state.bLeftTrigger; goto eitherTrigger;
		case RightTrigger: value = gamepad.state.bRightTrigger; goto eitherTrigger;
		eitherTrigger:
			if (ImGui::Button(name, ImVec2(80, 20)))
				feeder->StartRebindX360Mapping(gamepadId, btn);
			ImGui::SameLine();
			ImGui::Text("%d", value);
			break;

		case LStickUp: f1 = gamepad.state.sThumbLX; f2 = gamepad.state.sThumbLY; goto eitherStick;
		case RStickUp: f1 = gamepad.state.sThumbRX; f2 = gamepad.state.sThumbRY; goto eitherStick;
		eitherStick:
			ImGui::Text("%f", f1 / MAXSHORT);
			ImGui::SameLine();
			ImGui::Text("%f", f2 / MAXSHORT);
			ImGui::SameLine();
			if (ImGui::Button(name, ImVec2(80, 20)))
				feeder->StartRebindX360Mapping(gamepadId, btn);
			break;

		default:
			if (ImGui::Button(name, ImVec2(80, 20)))
				feeder->StartRebindX360Mapping(gamepadId, btn);
			break;
		}
	}
}
