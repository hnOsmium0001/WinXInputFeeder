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

struct HostWindow {
	std::string nameUtf8;
	HWND hwnd;
};

struct UIStatePrivate {
	int selectedUserIndex = -1;
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
	for (int gamepadId = 0; gamepadId < s.x360s.size(); ++gamepadId) {
		FORMAT_GAMEPAD_NAME(id, gamepadId);
		bool selected = p.selectedUserIndex == gamepadId;
		if (ImGui::Selectable(id, &selected)) {
			p.selectedUserIndex = gamepadId;
		}
	}
	ImGui::End();

	ImGui::Begin("Gamepad info");
	if (p.selectedUserIndex != -1) {
		auto userIndex = p.selectedUserIndex;
		auto& dev = s.x360s[userIndex];
		auto&& [profileName, profile] = *s.config.x360s[userIndex];

		if (ImGui::Button("Rebind")) {
			s.StartRebindX360Device(userIndex);
		}
		if (dev.pendingRebindDevice) {
			ImGui::SameLine();
			ImGui::Text("Rebinding: press any key or mosue button");
		}

		if (ImGui::Button("Unbind##kdb")) {
			s.x360s[userIndex].srcKbd = INVALID_HANDLE_VALUE;
		}
		ImGui::SameLine();
		if (dev.srcKbd == INVALID_HANDLE_VALUE)
			ImGui::Text("Bound keyboard: [any]");
		else
			ImGui::Text("Bound keyboard: %p", dev.srcKbd);

		if (ImGui::Button("Unbind##mouse")) {
			s.x360s[userIndex].srcMouse = INVALID_HANDLE_VALUE;
		}
		ImGui::SameLine();
		if (dev.srcMouse == INVALID_HANDLE_VALUE)
			ImGui::Text("Bound mouse: [any]");
		else
			ImGui::Text("Bound mouse: %p", dev.srcMouse);

		if (ImGui::BeginCombo("Profile name", profileName.c_str())) {
			for (auto& p : s.config.profiles) {
				bool selected = &p.second == &profile;
				if (ImGui::MenuItem(p.first.c_str(), nullptr, selected)) {
					LOG_DEBUG(L"UI: rebound gamepad {} to profile '{}'", userIndex, Utf8ToWide(profileName));
					s.SetX360Profile(userIndex, &p);
				}
			}
			ImGui::EndCombo();
		}

#define GAMEPAD_BTN_SIZED(NAME, VIGEM_ENUM, OUR_ENUM, BTN_SIZE) \
	if (dev.GetButton(VIGEM_ENUM)) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); \
	if (ImGui::Button(NAME, BTN_SIZE)) s.StartRebindX360Mapping(userIndex, OUR_ENUM); \
	if (dev.GetButton(VIGEM_ENUM)) ImGui::PopStyleColor()
#define GAMEPAD_BTN(NAME, VIGEM_ENUM, OUR_ENUM) GAMEPAD_BTN_SIZED(NAME, VIGEM_ENUM, OUR_ENUM, ImVec2(60, 20))

		if (ImGui::Button("LS Left", ImVec2(50, 20))) s.StartRebindX360Mapping(userIndex, XiButton::LStickLeft);
		ImGui::SameLine();
		if (ImGui::Button("LS Right", ImVec2(50, 20))) s.StartRebindX360Mapping(userIndex, XiButton::LStickRight);
		ImGui::SameLine();
		if (ImGui::Button("LS Up", ImVec2(50, 20))) s.StartRebindX360Mapping(userIndex, XiButton::LStickUp);
		ImGui::SameLine();
		if (ImGui::Button("LS Down", ImVec2(50, 20))) s.StartRebindX360Mapping(userIndex, XiButton::LStickDown);

		if (ImGui::Button("RS Left", ImVec2(50, 20))) s.StartRebindX360Mapping(userIndex, XiButton::RStickLeft);
		ImGui::SameLine();
		if (ImGui::Button("RS Right", ImVec2(50, 20))) s.StartRebindX360Mapping(userIndex, XiButton::RStickRight);
		ImGui::SameLine();
		if (ImGui::Button("RS Up", ImVec2(50, 20))) s.StartRebindX360Mapping(userIndex, XiButton::RStickUp);
		ImGui::SameLine();
		if (ImGui::Button("RS Down", ImVec2(50, 20))) s.StartRebindX360Mapping(userIndex, XiButton::RStickDown);

		GAMEPAD_BTN("L Thumb", XUSB_GAMEPAD_LEFT_THUMB, XiButton::LStickBtn);
		ImGui::SameLine();
		GAMEPAD_BTN("R Thumb", XUSB_GAMEPAD_RIGHT_THUMB, XiButton::RStickBtn);

		GAMEPAD_BTN("DP Left", XUSB_GAMEPAD_DPAD_LEFT, XiButton::DpadLeft);
		ImGui::SameLine();
		GAMEPAD_BTN("DP Right", XUSB_GAMEPAD_DPAD_RIGHT, XiButton::DpadRight);
		ImGui::SameLine();
		GAMEPAD_BTN("DP Up", XUSB_GAMEPAD_DPAD_UP, XiButton::DpadUp);
		ImGui::SameLine();
		GAMEPAD_BTN("DP Down", XUSB_GAMEPAD_DPAD_DOWN, XiButton::DpadDown);

		GAMEPAD_BTN("A", XUSB_GAMEPAD_A, XiButton::A);
		ImGui::SameLine();
		GAMEPAD_BTN("B", XUSB_GAMEPAD_B, XiButton::B);
		ImGui::SameLine();
		GAMEPAD_BTN("X", XUSB_GAMEPAD_X, XiButton::X);
		ImGui::SameLine();
		GAMEPAD_BTN("Y", XUSB_GAMEPAD_Y, XiButton::Y);

		GAMEPAD_BTN("LB", XUSB_GAMEPAD_LEFT_SHOULDER, XiButton::LB);
		ImGui::SameLine();
		GAMEPAD_BTN("RB", XUSB_GAMEPAD_RIGHT_SHOULDER, XiButton::RB);

		if (ImGui::Button("LT", ImVec2(60, 20))) s.StartRebindX360Mapping(userIndex, XiButton::LT);
		ImGui::SameLine();
		ImGui::Text("%d", dev.state.bLeftTrigger);

		if (ImGui::Button("RT", ImVec2(60, 20))) s.StartRebindX360Mapping(userIndex, XiButton::RT);
		ImGui::SameLine();
		ImGui::Text("%d", dev.state.bRightTrigger);

		GAMEPAD_BTN("Start", XUSB_GAMEPAD_START, XiButton::Start);
		ImGui::SameLine();
		GAMEPAD_BTN("Back", XUSB_GAMEPAD_BACK, XiButton::Back);

#undef GAMEPAD_BTN
#undef GAMEPAD_BTN_SIZED
	}
	else {
		ImGui::Text("Select a gamepad to show details");
	}
	ImGui::End();

	if (p.showDemoWindow) {
		ImGui::ShowDemoWindow(&p.showDemoWindow);
	}
}
