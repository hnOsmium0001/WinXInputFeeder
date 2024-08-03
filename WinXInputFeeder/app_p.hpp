#pragma once

#include "modelconfig.hpp"
#include "modelruntime.hpp"
#include "inputdevice.hpp"
#include "ui.hpp"

#include <ViGEm/Client.h>

#include <memory>

class App;

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

	MainWindow(App& app, HINSTANCE hInstance);
	~MainWindow();

	void CreateRenderTarget();
	void DestroyRenderTarget() noexcept;
	// Resize the render target after initial creation
	// NOTE: at initial creation, the size is automatically pulled from the specified HWND by D3D
	void ResizeRenderTarget(UINT width, UINT height);
};

LRESULT CALLBACK MainWindowWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept;

class App {
public:
	HINSTANCE hInstance;

	MainWindow mainWindow;
	UIState mainUI;

	ViGEm vigem;

	std::unique_ptr<FeederEngine> feeder;

	// For a RAWINPUT*
	// We have to use a manually sized buffer, because RAWINPUT uses a flexible array member at the end
	std::unique_ptr<std::byte[]> rawinput;
	size_t rawinputSize = 0;

	int shownWindowCount = 0;
	bool configDirty = false;
	bool capturingCursor = false;

public:
	App(HINSTANCE hInstance);
	~App();

	void MainRenderFrame();

	LRESULT OnRawInput(RAWINPUT*);
};
