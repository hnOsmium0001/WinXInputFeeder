#include "pch.hpp"

#include "ui.hpp"

#include "gamepad.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

using namespace std::literals;

#define FORMAT_GAMEPAD_NAME(VAR, USER_INDEX ) char VAR[256]; snprintf(VAR, sizeof(VAR), "Gamepad %d", (int)USER_INDEX);

struct HostWindow {
    std::string nameUtf8;
    HWND hwnd;
};

struct UIStatePrivate {
    std::vector<HostWindow> hostWindowList;
    int selectedUserIndex = -1;
    bool showDemoWindow = false;

    UIStatePrivate(UIState& s)
    {
    }

    void EnumHostWindows() {
        hostWindowList.clear();

        DWORD currProcessId = GetCurrentProcessId();
        HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (handle == INVALID_HANDLE_VALUE)
            return;
        DEFER{ CloseHandle(handle); };

        THREADENTRY32 te32;
        te32.dwSize = sizeof(te32);

        if (!Thread32First(handle, &te32))
            return;

        do {
            if (te32.th32OwnerProcessID == currProcessId) {
                EnumThreadWindows(te32.th32ThreadID, EnumThreadWindowsCallback, reinterpret_cast<LPARAM>(this));
            }
        } while (Thread32Next(handle, &te32));
    }

private:
    static BOOL CALLBACK EnumThreadWindowsCallback(HWND hwnd, LPARAM lParam) {
        auto self = reinterpret_cast<UIStatePrivate*>(lParam);

        WCHAR nameWide[256];
        int res = GetWindowTextW(hwnd, nameWide, IM_ARRAYSIZE(nameWide));
        std::string nameUtf8 = res == 0
            ? "<no name>"s
            : WideToUtf8(std::wstring_view(nameWide, res));

        self->hostWindowList.push_back(HostWindow{ std::move(nameUtf8), hwnd });

        return true;
    }
};

void ShowUI(UIState& s) {
    if (s.p == nullptr) {
        void* p = new UIStatePrivate(s);
        void(*deleter)(void*) = [](void* p) { delete (UIStatePrivate*)p; };
        s.p = { p, deleter };
    }
    auto& p = *static_cast<UIStatePrivate*>(s.p.get());

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
            s.bindIdevFromNextKey = userIndex;
        }
        ImGui::SameLine();
        if (ImGui::Button("Unbind##kdb")) {
            SrwExclusiveLock lock(gXiGamepadsLock);
            gXiGamepads[userIndex].srcKbd = INVALID_HANDLE_VALUE;
        }
        ImGui::SameLine();
        if (s.bindIdevFromNextKey == userIndex)
            ImGui::Text("Bound keyboard: [press any key]");
        else
            if (gXiGamepads[userIndex].srcKbd == INVALID_HANDLE_VALUE)
                ImGui::Text("Bound keyboard: [any]");
            else
                ImGui::Text("Bound keyboard: %p", gXiGamepads[userIndex].srcKbd);

        if (ImGui::Button("Rebind##mouse")) {
            s.bindIdevFromNextMouse = userIndex;
        }
        ImGui::SameLine();
        if (ImGui::Button("Unbind##mouse")) {
            SrwExclusiveLock lock(gXiGamepadsLock);
            gXiGamepads[userIndex].srcMouse = INVALID_HANDLE_VALUE;
        }
        ImGui::SameLine();
        if (s.bindIdevFromNextMouse == userIndex)
            ImGui::Text("Bound mouse: [press any mouse button]");
        else
            if (gXiGamepads[userIndex].srcMouse == INVALID_HANDLE_VALUE)
                ImGui::Text("Bound mouse: [any]");
            else
                ImGui::Text("Bound mouse: %p", gXiGamepads[userIndex].srcMouse);

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

    ImGui::Begin("Game");
    if (ImGui::Button("Enumerate host windows")) {
        p.EnumHostWindows();
    }
    for (const auto& hostWindow : p.hostWindowList) {
        char name[256];
        snprintf(name, IM_ARRAYSIZE(name), "%p %s", hostWindow.hwnd, hostWindow.nameUtf8.c_str());

        bool selected = hostWindow.hwnd == s.mainHostHwnd;
        if (ImGui::Selectable(name, &selected)) {
            s.mainHostHwnd = hostWindow.hwnd;
        }
    }
    ImGui::End();

    if (p.showDemoWindow) {
        ImGui::ShowDemoWindow(&p.showDemoWindow);
    }
}
