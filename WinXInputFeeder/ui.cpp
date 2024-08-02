#include "pch.hpp"

#include "ui.hpp"

#include "inputsrc.hpp"
#include "inputsrc_p.hpp" // TODO
#include "gamepad.hpp"
#include "utils.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

using namespace std::literals;

#define FORMAT_GAMEPAD_NAME(VAR, USER_INDEX ) char VAR[256]; snprintf(VAR, sizeof(VAR), "Gamepad %d", (int)USER_INDEX);

struct UIStatePrivate {
	int selectedGamepadId = -1;
	bool showDemoWindow = false;

	UIStatePrivate(UIState& s)
	{
	}
};

UIState::UIState(AppState& app)
	: p{ new UIStatePrivate(*this) }
	, app{ &app } {}

UIState::~UIState() {
	delete p;
}

static void ShowButton(AppState& app, X360Gamepad& gamepad, int gamepadId, X360Button btn, KeyCode boundKey) {
	using enum X360Button;

	const char* name = X360ButtonToString(btn).data();

	if (IsX360ButtonDirectMap(btn)) {
		bool pressed = gamepad.GetButton(X360ButtonToViGEm(btn));
		if (pressed) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
		if (ImGui::Button(name, ImVec2(80, 20)))
			app.StartRebindX360Mapping(gamepadId, btn);
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
				app.StartRebindX360Mapping(gamepadId, btn);
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
				app.StartRebindX360Mapping(gamepadId, btn);
			break;

		default:
			if (ImGui::Button(name, ImVec2(80, 20)))
				app.StartRebindX360Mapping(gamepadId, btn);
			break;
		}

	}
}

void UIState::Show() {
	auto& s = *app;
	auto& p = *static_cast<UIStatePrivate*>(this->p);

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("WinXInputEmu")) {
			if (ImGui::MenuItem("Reload config file")) {
				s.ReloadConfig();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Quit")) {
				PostQuitMessage(0);
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Tools")) {
			ImGui::MenuItem("ImGui demo window", nullptr, &p.showDemoWindow);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	ImGui::Begin("Gamepads");

	if (ImGui::BeginCombo("Profile", s.currentProfile->first.c_str())) {
		for (auto& it : s.config.profiles) {
			auto&& [profileName, profile] = it;
			bool selected = &it == s.currentProfile;
			if (ImGui::MenuItem(profileName.c_str(), nullptr, &selected)) {
				s.SelectProfile(&it);
			}
		}
		ImGui::EndCombo();
	}

	if (ImGui::Button("+")) {
		s.AddX360();
	}
	ImGui::SameLine();
	if (p.selectedGamepadId == -1) ImGui::BeginDisabled();
	if (ImGui::Button("-")) {
		s.RemoveGamepad(p.selectedGamepadId);
	}
	if (p.selectedGamepadId == -1) ImGui::EndDisabled();

	for (int gamepadId = 0; gamepadId < s.x360s.size(); ++gamepadId) {
		FORMAT_GAMEPAD_NAME(id, gamepadId);
		bool selected = p.selectedGamepadId == gamepadId;
		if (ImGui::Selectable(id, &selected)) {
			p.selectedGamepadId = gamepadId;
		}
	}
	ImGui::End();

	ImGui::Begin("Gamepad info");
	if (p.selectedGamepadId != -1) {
		auto gamepadId = p.selectedGamepadId;
		auto& currentProfile = s.currentProfile->second;
		auto& dev = s.x360s[gamepadId];
		auto& profile= currentProfile.gamepads[gamepadId];

		if (ImGui::Button("Rebind")) {
			s.StartRebindX360Device(gamepadId);
		}
		if (dev.pendingRebindDevice) {
			ImGui::SameLine();
			ImGui::Text("Rebinding: press any key or mosue button");
		}

		if (ImGui::Button("Unbind##kdb")) {
			s.x360s[gamepadId].srcKbd = INVALID_HANDLE_VALUE;
		}
		ImGui::SameLine();
		if (dev.srcKbd == INVALID_HANDLE_VALUE)
			ImGui::Text("Bound keyboard: [any]");
		else
			ImGui::Text("Bound keyboard: %p", dev.srcKbd);

		if (ImGui::Button("Unbind##mouse")) {
			s.x360s[gamepadId].srcMouse = INVALID_HANDLE_VALUE;
		}
		ImGui::SameLine();
		if (dev.srcMouse == INVALID_HANDLE_VALUE)
			ImGui::Text("Bound mouse: [any]");
		else
			ImGui::Text("Bound mouse: %p", dev.srcMouse);

		using enum X360Button;
#define BUTTON(THE_BTN) ShowButton(s, dev, gamepadId, THE_BTN, profile.buttons[static_cast<unsigned char>(THE_BTN)])
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
	else {
		ImGui::Text("Select a gamepad to show details");
	}
	ImGui::End();

	if (p.showDemoWindow) {
		ImGui::ShowDemoWindow(&p.showDemoWindow);
	}
}
