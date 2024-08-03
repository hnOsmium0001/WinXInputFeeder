#include "pch.hpp"

#include "app.hpp"
#include "app_p.hpp"

#include "modelconfig.hpp"
#include "modelruntime.hpp"
#include "inputdevice.hpp"
#include "ui.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <d3d11.h>
#include <filesystem>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <memory>
#include <utility>
#include <vector>
#include <hidusage.h>

namespace fs = std::filesystem;
using namespace std::literals;

constexpr UINT_PTR kMouseCheckTimerID = 0;

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK MainWindowWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	switch (uMsg) {
	case WM_NCCREATE: {
		auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
		break;
	}

	case WM_TIMER: {
		auto timerID = (UINT_PTR)wParam;

		//switch (timerID) {
		//case kMouseCheckTimerID: {
		//	DoMouse2Joystick(its);
		//	return 0;
		//}

		//default: break;
		//}
		break;
	}

	case WM_SIZE: {
		auto& app = *reinterpret_cast<App*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

		if (wParam == SIZE_MINIMIZED) {
			--app.shownWindowCount;
			return 0;
		}

		auto resizeWidth = static_cast<UINT>(LOWORD(lParam));
		auto resizeHeight = static_cast<UINT>(HIWORD(lParam));
		app.mainWindow.ResizeRenderTarget(resizeWidth, resizeHeight);
		++app.shownWindowCount;
		return 0;
	}

	case WM_DESTROY: {
		PostQuitMessage(0);
		return 0;
	}

	case WM_INPUT: {
		auto& app = *reinterpret_cast<App*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

		HRAWINPUT hri = (HRAWINPUT)lParam;

		UINT size = 0;
		GetRawInputData(hri, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
		if (size > app.rawinputSize || app.rawinput == nullptr) {
			app.rawinput = std::make_unique<std::byte[]>(size);
			app.rawinputSize = size;
		}

		if (GetRawInputData(hri, RID_INPUT, app.rawinput.get(), &size, sizeof(RAWINPUTHEADER)) == (UINT)-1) {
			LOG_DEBUG(L"GetRawInputData() failed");
			break;
		}
		RAWINPUT* ri = reinterpret_cast<RAWINPUT*>(app.rawinput.get());

		return app.OnRawInput(ri);
	}

	case WM_INPUT_DEVICE_CHANGE: {
		//HANDLE hDevice = (HANDLE)lParam;

		//if (wParam == GIDC_ARRIVAL) {
		//	// NOTE: WM_INPUT_DEVICE_CHANGE seem to fire when RIDEV_DEVNOTIFY is first set on a window, so we get duplicate devices here as the ones collected in _glfwPollKeyboardsWin32()
		//	// Filter duplicate devices
		//	for (const auto& idev : s.devices) {
		//		if (idev.hDevice == hDevice)
		//			return 0;
		//	}

		//	s.devices.push_back(IdevDevice::FromHANDLE(hDevice));

		//	const auto& idev = s.devices.back();
		//	LOG_DEBUG("Connected {} {}", RawInputTypeToString(idev.info.dwType), idev.nameWide);
		//}
		//else if (wParam == GIDC_REMOVAL) {
		//	// HACK: this relies on std::erase_if only visiting each element once (which is almost necessarily the case) but still technically not standard-compliant
		//	//       for the behavior of "log the device being removed once"
		//	std::erase_if(
		//		s.devices,
		//		[&](const IdevDevice& idev) {
		//			if (idev.hDevice == hDevice) {
		//				LOG_DEBUG("Disconnected {} {}", RawInputTypeToString(idev.info.dwType), idev.nameWide);
		//				return true;
		//			}
		//			else {
		//				return false;
		//			}
		//		});
		//}

		//return 0;
	}
	}

	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

MainWindow::MainWindow(App& app, HINSTANCE hInstance) {
	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = MainWindowWndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"WinXInputFeeder Config";
	hWc = RegisterClassExW(&wc);
	if (!hWc)
		throw std::runtime_error(std::format("Error creating main window class: {}", GetLastErrorStrUtf8()));

	hWnd = CreateWindowExW(
		0,
		MAKEINTATOM(hWc),
		L"WinXInputFeeder Config",
		WS_OVERLAPPEDWINDOW,

		// Position
		CW_USEDEFAULT, CW_USEDEFAULT,
		// Size
		1024, 640,

		NULL,  // Parent window    
		NULL,  // Menu
		NULL,  // Instance handle
		reinterpret_cast<LPVOID>(&app)
	);
	if (hWnd == nullptr)
		throw std::runtime_error(std::format("Error creating main window: {}", GetLastErrorStrUtf8()));

	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	D3D_FEATURE_LEVEL featureLevelArray[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &swapChain, &d3dDevice, &featureLevel, &d3dDeviceContext);
	// Try high-performance WARP software driver if hardware is not available.
	if (res == DXGI_ERROR_UNSUPPORTED)
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &swapChain, &d3dDevice, &featureLevel, &d3dDeviceContext);
	if (res != S_OK)
		throw std::runtime_error("Failed to create D3D device and swapchain");

	CreateRenderTarget();
}

MainWindow::~MainWindow() {
	DestroyRenderTarget();
	if (swapChain) { swapChain->Release(); }
	if (d3dDeviceContext) { d3dDeviceContext->Release(); }
	if (d3dDevice) { d3dDevice->Release(); }

	DestroyWindow(hWnd);
	UnregisterClassW(MAKEINTATOM(hWc), nullptr);
}

void MainWindow::CreateRenderTarget() {
	ID3D11Texture2D* backBuffer;
	swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &mainRenderTargetView);
	backBuffer->Release();
}

void MainWindow::DestroyRenderTarget() {
	if (mainRenderTargetView)
		mainRenderTargetView->Release();
}

void MainWindow::ResizeRenderTarget(UINT width, UINT height) {
	DestroyRenderTarget();
	swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
	CreateRenderTarget();
}

static toml::table LoadConfigFile() {
	toml::table configFile;
	try {
		configFile = toml::parse_file(fs::path(L"config.toml"));
	}
	catch (const toml::parse_error& err) {
		return toml::table();
	}

	if (auto configAltPath = configFile["AltPath"].value<std::string>()) {
		// If parse error, let it propagate out
		return toml::parse_file(fs::path(*configAltPath));
	}
	return configFile;
}

static FeederEngine MakeFeederEngine(ViGEm& vigem) {
	FeederEngine engine(Config(LoadConfigFile()), vigem);
	return engine;
}

App::App(HINSTANCE hInstance)
	: hInstance{ hInstance }
	, mainWindow(*this, hInstance)
	, mainUI(*this)
	, feeder(std::make_unique<FeederEngine>(MakeFeederEngine(vigem)))
{
	mainUI.OnFeederEngine(feeder.get());

	constexpr UINT kNumRid = 2;
	RAWINPUTDEVICE rid[kNumRid];

	// We don't use RIDEV_NOLEGACY because all the window manipulation (e.g. dragging the title bar) relies on the "legacy messages"
	// RIDEV_INPUTSINK so that we get input even if the game window is current in focus instead
	rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
	rid[0].dwFlags = RIDEV_DEVNOTIFY | RIDEV_INPUTSINK;
	rid[0].usUsage = HID_USAGE_GENERIC_KEYBOARD;
	rid[0].hwndTarget = mainWindow.hWnd;

	rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
	rid[1].dwFlags = RIDEV_DEVNOTIFY | RIDEV_INPUTSINK;
	rid[1].usUsage = HID_USAGE_GENERIC_MOUSE;
	rid[1].hwndTarget = mainWindow.hWnd;

	if (RegisterRawInputDevices(rid, kNumRid, sizeof(RAWINPUTDEVICE)) == false)
		throw std::runtime_error("Failed to register RAWINPUT devices");

	ShowWindow(mainWindow.hWnd, SW_SHOWDEFAULT);
	UpdateWindow(mainWindow.hWnd);
	++shownWindowCount;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	auto& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.IniFilename = "imgui_state.ini";

	ImGui_ImplWin32_Init(mainWindow.hWnd);
	ImGui_ImplDX11_Init(mainWindow.d3dDevice, mainWindow.d3dDeviceContext);
}

App::~App() {
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void App::MainRenderFrame() {
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::DockSpaceOverViewport();
	mainUI.Show();

	ImGui::Render();
	constexpr ImVec4 kClearColor{ 0.45f, 0.55f, 0.60f, 1.00f };
	constexpr float kClearColorPremultAlpha[]{ kClearColor.x * kClearColor.w, kClearColor.y * kClearColor.w, kClearColor.z * kClearColor.w, kClearColor.w };
	ID3D11DeviceContext* devCtx = mainWindow.d3dDeviceContext;
	ID3D11RenderTargetView* rtv = mainWindow.mainRenderTargetView;
	devCtx->OMSetRenderTargets(1, &rtv, nullptr);
	devCtx->ClearRenderTargetView(rtv, kClearColorPremultAlpha);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	mainWindow.swapChain->Present(1, 0); // Present with vsync
}

LRESULT App::OnRawInput(RAWINPUT* ri) {
	switch (ri->header.dwType) {
	case RIM_TYPEMOUSE: {
		const auto& mouse = ri->data.mouse;

		auto bf = mouse.usButtonFlags;
		auto dev = ri->header.hDevice;
		if (bf & RI_MOUSE_LEFT_BUTTON_DOWN) feeder->HandleKeyPress(dev, VK_LBUTTON, true);
		if (bf & RI_MOUSE_LEFT_BUTTON_UP) feeder->HandleKeyPress(dev, VK_LBUTTON, false);
		if (bf & RI_MOUSE_RIGHT_BUTTON_DOWN) feeder->HandleKeyPress(dev, VK_RBUTTON, true);
		if (bf & RI_MOUSE_RIGHT_BUTTON_UP) feeder->HandleKeyPress(dev, VK_RBUTTON, false);
		if (bf & RI_MOUSE_MIDDLE_BUTTON_DOWN) feeder->HandleKeyPress(dev, VK_MBUTTON, true);
		if (bf & RI_MOUSE_MIDDLE_BUTTON_UP) feeder->HandleKeyPress(dev, VK_MBUTTON, false);
		if (bf & RI_MOUSE_BUTTON_4_DOWN) feeder->HandleKeyPress(dev, VK_XBUTTON1, true);
		if (bf & RI_MOUSE_BUTTON_4_UP) feeder->HandleKeyPress(dev, VK_XBUTTON1, false);
		if (bf & RI_MOUSE_BUTTON_5_DOWN) feeder->HandleKeyPress(dev, VK_XBUTTON2, true);
		if (bf & RI_MOUSE_BUTTON_5_UP) feeder->HandleKeyPress(dev, VK_XBUTTON2, false);

		if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
			LOG_DEBUG("Warning: RAWINPUT reported absolute mouse corrdinates, not supported");
			break;
		} // else: MOUSE_MOVE_RELATIVE

		feeder->HandleMouseMovement(ri->header.hDevice, mouse.lLastX, mouse.lLastY);
	} break;

	case RIM_TYPEKEYBOARD: {
		const auto& kbd = ri->data.keyboard;

		// This message is a part of a longer makecode sequence -- the actual Vkey is in another one
		if (kbd.VKey == 0xFF)
			break;
		// All of the relevant keys that we support fit in a BYTE
		if (kbd.VKey > 0xFF)
			break;

		bool press = !(kbd.Flags & RI_KEY_BREAK);
		feeder->HandleKeyPress(ri->header.hDevice, (BYTE)kbd.VKey, press);
	} break;
	}

	return 0;
}

int AppMain(HINSTANCE hInstance, std::span<const std::wstring_view> args) {
	App s(hInstance);

	while (true) {
		MSG msg;

		// The blocking message pump
		// We'll block here, until one of the messages changes changes blockingMessagePump to false (i.e. we should be rendering again) ...
		while (s.shownWindowCount == 0 && GetMessageW(&msg, nullptr, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
			if (msg.message == WM_QUIT)
				goto exit;
		}

		// ... in which case the above loop breaks, and we come here (regular polling message pump) to process the rest, and then enter regular main loop doing rendering + polling
		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);

			// WM_QUIT is gaurenteed to only exist when there is nothing else in the message queue, we can safely exit immediately
			if (msg.message == WM_QUIT)
				goto exit;
		}

		s.MainRenderFrame();
	}
exit:
	return 0;
}
