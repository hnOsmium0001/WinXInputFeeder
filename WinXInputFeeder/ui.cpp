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
	}
	else {
		ImGui::Text("Select a gamepad to show details");
	}
	ImGui::End();

	if (p.showDemoWindow) {
		ImGui::ShowDemoWindow(&p.showDemoWindow);
	}
}
