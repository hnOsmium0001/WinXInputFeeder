#include "pch.hpp"

#include "ui.hpp"

#include "app.hpp"
#include "modelruntime.hpp"
#include "utils.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <string>

using namespace std::literals;

constexpr auto kLabelWidth = 80.0f;
constexpr auto kButtonSize = ImVec2(80.0f, 20.0f);

static void HelpForItem(const char* desc) {
	if (ImGui::BeginItemTooltip()) {
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

// Helper to display a little (?) mark which shows a tooltip when hovered.
// In your own code you may want to display an actual icon if you are using a merged icon fonts (see docs/FONTS.md)
static void HelpMarker(const char* desc) {
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	HelpForItem(desc);
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

static void DrawJoystickCircle(ImGuiID id, float radius, SHORT x, SHORT y) {
	auto window = ImGui::GetCurrentWindow();
	float cursorX = window->DC.CursorPos.x;
	float cursorY = window->DC.CursorPos.y;
	ImRect rect{ cursorX, cursorY, cursorX + radius*2, cursorY + radius*2 };

	ImGui::ItemSize(rect);
	if (!ImGui::ItemAdd(rect, id))
		return;

	ImVec2 center{ cursorX + radius, cursorY + radius };

	float normX = static_cast<float>(x) / MAXSHORT;
	float normY = -static_cast<float>(y) / MAXSHORT;
	ImVec2 pt{ center.x + normX * radius, center.y + normY * radius };

	if (float dist = ImSqrt(normX*normX + normY*normY); dist > 1.0f) {
		normX = normX / dist;
		normY = normY / dist;
	}
	ImVec2 ptOnCircle{ center.x + normX * radius, center.y + normY * radius };

	ImU32 borderColor = ImGui::GetColorU32(ImGuiCol_Button);
	ImU32 ptColor = IM_COL32(255, 0, 0, 255);
	ImU32 circleColor = IM_COL32(255, 247, 184, 255);
	ImU32 rayColor = IM_COL32(255, 255, 0, 255);

	auto cdl = window->DrawList;
	cdl->AddRect(rect.Min, rect.Max, borderColor, 0.0f, 0, 2.0f);
	cdl->AddCircle(center, radius, circleColor);
	cdl->AddLine(center, pt, rayColor);
	cdl->AddCircleFilled(ptOnCircle, 3.0f, rayColor);
	cdl->AddCircleFilled(pt, 4.0f, ptColor);
}

static void ShowTextForKey(KeyCode boundKey, bool pressed) {
	auto boundKeyName = boundKey == 0xFF ? "" : KeyCodeToString(boundKey).data();
	ImGui::SetNextItemWidth(kLabelWidth);
	if (pressed) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
	ImGui::TextUnformatted(boundKeyName);
	if (pressed) ImGui::PopStyleColor();
}

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
		--selectedGamepadId;
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

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	using enum X360Button;

	auto ShowButton = [&](X360Button btn) {
		auto btnName = X360ButtonToString(btn).data();
		if (ImGui::Button(btnName, kButtonSize))
			feeder->StartRebindX360Mapping(selectedGamepadId, btn);

		ImGui::SameLine();
		ShowTextForKey(
			/*button*/  gamepad.buttons[static_cast<unsigned char>(btn)],
			/*pressed*/ dev.GetButton(X360ButtonToViGEm(btn)));
		};

	auto ShowTrigger = [&](X360Button btn, BYTE triggerValue) {
		const char* btnName = X360ButtonToString(btn).data();
		if (ImGui::Button(btnName, kButtonSize))
			feeder->StartRebindX360Mapping(selectedGamepadId, btn);

		ImGui::SameLine();
		ImGui::ProgressBar(static_cast<float>(triggerValue) / MAXBYTE, ImVec2(kLabelWidth, 0));
		};

	auto ShowStick = [&](bool leftright, SHORT x, SHORT y) {
		X360Button thumbBtn = leftright ? RightThumb : LeftThumb;
		X360Button stickBtn1st = leftright ? RStickUp : LStickUp;
		auto stickBtn1stIdx = static_cast<unsigned char>(stickBtn1st);

		auto& opts = feeder->GetX360JoystickParams(selectedGamepadId, leftright);

		ImGui::PushID(leftright);
		
		ImGui::BeginGroup(); ImGui::PushItemWidth(kLabelWidth);

		// 1st item
		ShowButton(thumbBtn);

		// 2nd item - toggle between mouse and keyboard
		bool useMouse = opts.useMouse;
		if (ImGui::Checkbox("Mouse?", &useMouse)) {
			feeder->SetX360JoystickMode(selectedGamepadId, leftright, useMouse);
		}
		ImGui::Spacing();

		// rest items
		if (useMouse) {
			ImGui::InputFloat("Sensitivity", &opts.sensitivity);
			HelpForItem("Lower value corresponds to higher sensitivity.");

			ImGui::SliderFloat("Non-Linear", &opts.nonLinear, 0.0f, 1.0f);
			HelpForItem("1.0 is linear\n< 1.0 makes center more sensitive");

			ImGui::SliderFloat("Deadzone", &opts.deadzone, 0.0f, 1.0f);

			ImGui::Checkbox("Invert X-Axis", &opts.invertXAxis);

			ImGui::Checkbox("Invert Y-Axis", &opts.invertYAxis);
		}
		else {
			ImGui::PushItemWidth(kLabelWidth);
			float speedPercent = opts.speed * 100;
			ImGui::SliderFloat("Speed", &speedPercent, 0.0f, 100.0f, "%.0f%%");
			opts.speed = speedPercent / 100;

			for (unsigned char i = 0; i < 4; ++i) {
				auto btn = static_cast<X360Button>(stickBtn1stIdx + i);
				auto btnName = X360ButtonToString(btn).data();
				if (ImGui::Button(btnName, kButtonSize))
					feeder->StartRebindX360Mapping(selectedGamepadId, btn);

				ImGui::SameLine();
				auto boundKey = gamepad.buttons[stickBtn1stIdx + i];
				auto boundKeyName = boundKey == 0xFF ? "" : KeyCodeToString(boundKey).data();
				ImGui::SetNextItemWidth(kLabelWidth);
				ImGui::TextUnformatted(boundKeyName);
			}
		}
		ImGui::PopItemWidth(); ImGui::EndGroup();

		ImGui::SameLine();
		DrawJoystickCircle(ImGui::GetID("js"), 60.0f, x, y);
		ImGui::PopID();
		};
	ShowStick(false, dev.state.sThumbLX, dev.state.sThumbLY);
	ImGui::SameLine();
	ShowStick(true, dev.state.sThumbRX, dev.state.sThumbRY);

	ImGui::Spacing();

	ImGui::BeginGroup();
	ShowButton(DPadUp);
	ShowButton(DPadDown);
	ShowButton(DPadLeft);
	ShowButton(DPadRight);
	ImGui::EndGroup();
	ImGui::SameLine(); ImGui::BeginGroup();
	ShowButton(Start);
	ShowButton(Back);
	ShowButton(Guide);
	ImGui::EndGroup();
	ImGui::SameLine(); ImGui::BeginGroup();
	ShowButton(A);
	ShowButton(B);
	ShowButton(X);
	ShowButton(Y);
	ImGui::EndGroup();

	ImGui::Spacing();

	ImGui::BeginGroup();
	ShowButton(LeftShoulder);
	ShowButton(RightShoulder);
	ImGui::EndGroup();

	ImGui::BeginGroup();
	ShowTrigger(LeftTrigger, dev.state.bLeftTrigger);
	ShowTrigger(RightTrigger, dev.state.bRightTrigger);
	ImGui::EndGroup();
}

void UIStatePrivate::ShowButton(const X360Gamepad& gamepad, int gamepadId, X360Button btn, KeyCode boundKey) {
	using enum X360Button;

}
