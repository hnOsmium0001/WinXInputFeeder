#include "pch.hpp"

#include "ui.hpp"

#include "inputsrc.hpp"
#include "inputsrc_p.hpp" // TODO
#include "modelruntime.hpp"
#include "utils.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <string>

using namespace std::literals;

#define FORMAT_GAMEPAD_NAME(VAR, USER_INDEX) char VAR[256]; snprintf(VAR, sizeof(VAR), "Gamepad %d", (int)USER_INDEX);

struct UIStatePrivate {
	UIState* pub;
	FeederEngine* feeder = nullptr;
	std::string newProfileName;
	int selectedGamepadId = -1;
	bool showDemoWindow = false;

	UIStatePrivate(UIState& s)
		: pub{ &s }
	{
	}

	void Show();
	void ShowNavWindow();
	void ShowDetailWindow();

	void ShowButton(const X360Gamepad& gamepad, int gamepadId, X360Button btn, KeyCode boundKey);
};

UIState::UIState(AppState& app)
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
		if (ImGui::BeginMenu("Tools")) {
			ImGui::MenuItem("ImGui demo window", nullptr, &showDemoWindow);
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

	if (showDemoWindow) {
		ImGui::ShowDemoWindow(&showDemoWindow);
	}
}

void UIStatePrivate::ShowNavWindow() {
	auto& config = feeder->GetConfig();

	auto pp = feeder->GetCurrentProfile();
	if (ImGui::Button("+profile")) {
		newProfileName.clear();
		ImGui::OpenPopup("Create Profile");
	}
	ImGui::SameLine();
	if (!pp) ImGui::BeginDisabled();
	if (ImGui::Button("-profile")) {
		feeder->RemoveProfile(pp);
	}
	if (!pp) ImGui::EndDisabled();

	if (ImGui::BeginPopup("Create Profile")) {
		ImGui::InputText("Name", &newProfileName);
		if (ImGui::Button("Confirm")) {
			feeder->AddProfile(newProfileName);
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (!pp) {
		if (ImGui::BeginCombo("Profile", "Create a profile")) {
			ImGui::MenuItem("Create a profile by clicking the +profile button above", nullptr, nullptr, false);
			ImGui::EndCombo();
		}
		return;
	}

	const std::string& profileName = pp->first;
	const ConfigProfile& profile = pp->second;
	auto x360s = feeder->GetX360s();

	if (ImGui::BeginCombo("Profile", profileName.c_str())) {
		for (auto& it : config.profiles) {
			auto&& [profileName, profile] = it;
			bool selected = &it == pp;
			if (ImGui::MenuItem(profileName.c_str(), nullptr, &selected)) {
				feeder->SelectProfile(&it);
			}
		}
		ImGui::EndCombo();
	}

	if (ImGui::Button("+")) {
		feeder->AddX360();
	}
	ImGui::SameLine();
	if (selectedGamepadId == -1) ImGui::BeginDisabled();
	if (ImGui::Button("-")) {
		feeder->RemoveGamepad(selectedGamepadId);
	}
	if (selectedGamepadId == -1) ImGui::EndDisabled();

	for (int gamepadId = 0; gamepadId < x360s.size(); ++gamepadId) {
		FORMAT_GAMEPAD_NAME(id, gamepadId);
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
		ImGui::Text("Bound keyboard: [any]");
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
		ImGui::Text("Bound mouse: [any]");
	}
	else {
		ImGui::Text("Bound mouse: %p", dev.srcMouse);
	}

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
