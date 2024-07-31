#include "pch.hpp"

#include "ui.hpp"

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

UIState::UIState() 
	: p{ new UIStatePrivate(*this) } {}

UIState::~UIState() {
	delete p;
}

void UIState::Show() {
	//auto& s = ;
	auto& p = *static_cast<UIStatePrivate*>(this->p);

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("WinXInputEmu")) {
			if (ImGui::MenuItem("Reload config file")) {
				ReloadConfigFromDesignatedPath();
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
	for (int userIndex = 0; userIndex < 4; ++userIndex) {
		FORMAT_GAMEPAD_NAME(id, userIndex);
		bool selected = p.selectedUserIndex == userIndex;
		if (ImGui::Selectable(id, &selected)) {
			p.selectedUserIndex = userIndex;
		}
	}
	ImGui::End();

	ImGui::Begin("Gamepad info");
	if (p.selectedUserIndex != -1) {
		auto userIndex = p.selectedUserIndex;
		auto& profileName = gConfig.xiGamepadBindings[userIndex];

		if (ImGui::Button("Rebind##kdb")) {
			//s.bindIdevFromNextKey = userIndex;
		}
		ImGui::SameLine();
		if (ImGui::Button("Unbind##kdb")) {
			SrwExclusiveLock lock(gX360GamepadsLock);
			gX360Gamepads[userIndex].srcKbd = INVALID_HANDLE_VALUE;
		}
		ImGui::SameLine();
		//if (s.bindIdevFromNextKey == userIndex)
		//	ImGui::Text("Bound keyboard: [press any key]");
		//else
			if (gX360Gamepads[userIndex].srcKbd == INVALID_HANDLE_VALUE)
				ImGui::Text("Bound keyboard: [any]");
			else
				ImGui::Text("Bound keyboard: %p", gX360Gamepads[userIndex].srcKbd);

		if (ImGui::Button("Rebind##mouse")) {
			//s.bindIdevFromNextMouse = userIndex;
		}
		ImGui::SameLine();
		if (ImGui::Button("Unbind##mouse")) {
			SrwExclusiveLock lock(gX360GamepadsLock);
			gX360Gamepads[userIndex].srcMouse = INVALID_HANDLE_VALUE;
		}
		ImGui::SameLine();
		//if (s.bindIdevFromNextMouse == userIndex)
		//	ImGui::Text("Bound mouse: [press any mouse button]");
		//else
			if (gX360Gamepads[userIndex].srcMouse == INVALID_HANDLE_VALUE)
				ImGui::Text("Bound mouse: [any]");
			else
				ImGui::Text("Bound mouse: %p", gX360Gamepads[userIndex].srcMouse);

		if (ImGui::InputText("Profile name", &profileName)) {
			auto iter = gConfig.profiles.find(profileName);
			if (iter != gConfig.profiles.end()) {
				auto& profile = iter->second;

				LOG_DEBUG(L"UI: rebound gamepad {} to profile '{}'", userIndex, Utf8ToWide(profileName));
				BindProfileToGamepad(userIndex, profile);
				gConfigEvents.onGamepadBindingChanged(userIndex, profileName, profile);
			}
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
