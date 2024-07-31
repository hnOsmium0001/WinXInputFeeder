#pragma once

#include "inputdevice.hpp"
#include "userconfig.hpp"
#include "ui.hpp"

#include <ViGEm/Client.h>

struct AppState;

enum class XiButton : unsigned char {
	None = 0,
	A, B, X, Y,
	LB, RB,
	LT, RT,
	Start, Back,
	DpadUp, DpadDown, DpadLeft, DpadRight,
	LStickBtn, RStickBtn,
	LStickUp, LStickDown, LStickLeft, LStickRight,
	RStickUp, RStickDown, RStickLeft, RStickRight,
};

// Information and lookup tables computable from a Config object
// used for translating input key presses/mouse movements into gamepad state
struct InputTranslationStruct {
	struct {
		struct {
			// Keyboard mode stuff
			bool up, down, left, right;

			// Mouse mode stuff
		} lstick, rstick;

		float accuMouseX;
		float accuMouseY;
	} sticks[4];

	// VK_xxx is BYTE, max 255 values
	// NOTE: XiButton::COUNT is used to indicate "this mapping is not bound"
	XiButton btns[4][0xFF];

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

// Wrap as struct for RAII helper
struct ViGEm {
	PVIGEM_CLIENT hvigem;

	ViGEm();
	~ViGEm();
};

struct AppState {
	HINSTANCE hInstance;

	MainWindow mainWindow;
	UIState mainUI;

	std::vector<IdevDevice> devices;
	ViGEm vigem;

	InputTranslationStruct its;

	// For a RAWINPUT*
	// We have to use a manually sized buffer, because RAWINPUT uses a flexible array member at the end
	std::unique_ptr<std::byte[]> rawinput;
	size_t rawinputSize = 0;

	int shownWindowCount = 0;
	bool capturingCursor = false;

	AppState(HINSTANCE hInstance);
	~AppState();

	void MainRenderFrame();
};
