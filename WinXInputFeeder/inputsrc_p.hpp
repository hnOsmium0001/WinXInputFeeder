#pragma once

#include "gamepad.hpp"
#include "inputdevice.hpp"
#include "userconfig.hpp"
#include "ui.hpp"

#include <ViGEm/Client.h>

struct AppState;

// Information and lookup tables computable from a Config object
// used for translating input key presses/mouse movements into gamepad state
struct InputTranslationStruct {
	struct {
		float accuMouseX;
		float accuMouseY;
	} sticks[4];

	// VK_xxx is BYTE, max 255 values
	// NOTE: XiButton::COUNT is used to indicate "this mapping is not bound"
	X360Button btns[4][0xFF];

	InputTranslationStruct() {
		ClearAll();
	}

	void ClearAll();
	void PopulateBtnLut(int userIndex, const UserProfile& profile);
};

// Wrap as struct for RAII helper
// Not an encapsulated object in the OOP sense
struct MainWindow {
	HWND hWnd = nullptr;
	ATOM hWc = 0;
	/* 2-byte padding auto-added */
	
	// https://github.com/ocornut/imgui/blob/master/examples/example_win32_directx11/main.cpp
	// For ImGui main viewport
	ID3D11Device* d3dDevice = nullptr;
	ID3D11DeviceContext* d3dDeviceContext = nullptr;
	IDXGISwapChain* swapChain = nullptr;
	ID3D11RenderTargetView* mainRenderTargetView = nullptr;

	MainWindow(AppState& app, HINSTANCE hInstance);
	~MainWindow();

	void CreateRenderTarget();
	void DestroyRenderTarget();
	// Resize the render target after initial creation
	// NOTE: at initial creation, the size is automatically pulled from the specified HWND by D3D
	void ResizeRenderTarget(UINT width, UINT height);
};

LRESULT CALLBACK MainWindowWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept;

struct AppState {
	HINSTANCE hInstance;

	Config config;
	ViGEm vigem;
	std::vector<X360Gamepad> x360s;
	//std::vector<DualShockGamepad> dualshocks;
	InputTranslationStruct its;

	MainWindow mainWindow;
	UIState mainUI;

	std::vector<IdevDevice> devices;

	// For a RAWINPUT*
	// We have to use a manually sized buffer, because RAWINPUT uses a flexible array member at the end
	std::unique_ptr<std::byte[]> rawinput;
	size_t rawinputSize = 0;

	int shownWindowCount = 0;
	bool configDirty = false;
	bool capturingCursor = false;

	AppState(HINSTANCE hInstance);
	~AppState();

	// Functions to update the config & other states
	void ReloadConfig();
	void OnPostLoadConfig();
	void SetX360Profile(int gamepadId, Config::ProfileRef profile);
	void StartRebindX360Device(int gamepadId);
	void StartRebindX360Mapping(int gamepadId, X360Button btn);
	// TODO set joystick options in mouse mode
	void SetX360JoystickMode(int gamepadId, bool useRight /* false: left */, bool useMouse /* false: keyboard */);

	void MainRenderFrame();

	LRESULT OnRawInput(RAWINPUT*);
	void HandleKeyPress(HANDLE hDevice, BYTE vkey, bool pressed);
};
