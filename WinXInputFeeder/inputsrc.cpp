#include "pch.hpp"

#include "inputsrc.hpp"
#include "inputsrc_p.hpp"

#include "userconfig.hpp"
#include "utils.hpp"
#include "gamepad.hpp"
#include "inputdevice.hpp"
#include "ui.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <d3d11.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <memory>
#include <utility>
#include <vector>
#include <hidusage.h>

constexpr UINT_PTR kMouseCheckTimerID = 0;

void InputTranslationStruct::ClearAll() {
	for (int userIndex = 0; userIndex < 4; ++userIndex) {
		sticks[userIndex] = {};
		for (int i = 0; i < 0xFF; ++i) {
			btns[userIndex][i] = XiButton::None;
		}
	}
}

void InputTranslationStruct::PopulateBtnLut(int userIndex, const UserProfile& profile) {
	// Clear
	for (auto& btn : btns[userIndex])
		btn = XiButton::None;

	using enum XiButton;
#define BTN(KEY_ENUM, THE_BTN) if (THE_BTN.keyCode != 0xFF) btns[userIndex][THE_BTN.keyCode] = KEY_ENUM;
	BTN(A, profile.a);
	BTN(B, profile.b);
	BTN(X, profile.x);
	BTN(Y, profile.y);
	BTN(LB, profile.lb);
	BTN(RB, profile.rb);
	BTN(LT, profile.lt);
	BTN(RT, profile.rt);
	BTN(Start, profile.start);
	BTN(Back, profile.back);
	BTN(DpadUp, profile.dpadUp);
	BTN(DpadDown, profile.dpadDown);
	BTN(DpadLeft, profile.dpadLeft);
	BTN(DpadRight, profile.dpadRight);
	BTN(LStickBtn, profile.lstickBtn);
	BTN(RStickBtn, profile.rstickBtn);
#define STICK(PREFIX, THE_STICK) \
    if (THE_STICK.useMouse) {} \
    else { BTN(PREFIX##StickUp, THE_STICK.kbd.up); BTN(PREFIX##StickDown, THE_STICK.kbd.down); BTN(PREFIX##StickLeft, THE_STICK.kbd.left); BTN(PREFIX##StickRight, THE_STICK.kbd.right); }
	STICK(L, profile.lstick);
	STICK(R, profile.rstick);
#undef STICK
#undef BTN
}

static float Scale(float x, float lowerbound, float upperbound) {
	return (x - lowerbound) / (upperbound - lowerbound);
}

/// \param phi phi ∈ [0,2π], defines in which direction the stick is tilted.
/// \param tilt tilt ∈ (0,1], defines the amount of tilt. 0 is no tilt, 1 is full tilt.
static void SetJoystickPosition(float phi, float tilt, bool invertX, bool invertY, short& outX, short& outY) {
	constexpr float pi = 3.14159265358979323846f;
	constexpr float kSnapToFullFilt = 0.005f;

	tilt = std::clamp(tilt, 0.0f, 1.0f);
	tilt = (1 - tilt) < kSnapToFullFilt ? 1 : tilt;

	// [0,1]
	float x, y;

#define RANGE_FOR_X(LOWERBOUND, UPPERBOUND, FACTOR) if (phi >= LOWERBOUND && phi <= UPPERBOUND) { x = FACTOR * tilt * Scale(phi, LOWERBOUND, UPPERBOUND); y = FACTOR * tilt; }
#define RANGE_FOR_Y(LOWERBOUND, UPPERBOUND, FACTOR) if (phi >= LOWERBOUND && phi <= UPPERBOUND) { x = FACTOR * tilt; y = FACTOR * tilt * Scale(phi, LOWERBOUND, UPPERBOUND); }
	// Two cases with forward+right
	// Tilt is forward and slightly right
	RANGE_FOR_X(3 * pi / 2, 7 * pi / 4, 1);
	// Tilt is slightly forwardand right.
	RANGE_FOR_Y(7 * pi / 4, 2 * pi, 1);
	// Two cases with right+downward
	// Tilt is right and slightly downward.
	RANGE_FOR_Y(0, pi / 4, 1);
	// Tilt is downward and slightly right.
	RANGE_FOR_X(pi / 4, pi / 2, 1);
	// Two cases with downward+left
	// Tilt is downward and slightly left.
	RANGE_FOR_X(pi / 2, 3 * pi / 4, -1);
	// Tilt is left and slightly downward.
	RANGE_FOR_X(3 * pi / 4, pi, -1);
	// Two cases with forward+left
	// Tilt is left and slightly forward.
	RANGE_FOR_Y(pi, 5 * pi / 4, -1);
	// Tilt is forward and slightly left.
	RANGE_FOR_X(5 * pi / 4, 3 * pi / 2, -1);
#undef RANGE_FOR_X
#undef RANGE_FOR_Y


	// Scale [0,1] to [INT16_MIN,INT16_MAX]
	outX = static_cast<short>(x * 32768);
	outY = static_cast<short>(y * 32768);

	if (invertX) outX = -outX;
	if (invertY) outY = -outY;
}

//static void DoMouse2Joystick(InputTranslationStruct& its) {
//	for (int userIndex = 0; userIndex < 4; ++userIndex) {
//		auto& profile = gXiGamepads[userIndex].profile;
//		auto& dev = gXiGamepads[userIndex];
//		auto& extra = its.sticks[userIndex];
//
//		constexpr float kOuterRadius = 10.0f;
//		constexpr float kBounceBack = 0.0f;
//
//		float accuX = extra.accuMouseX;
//		float accuY = extra.accuMouseY;
//		// Distance of mouse from center
//		float r = sqrt(accuX * accuX + accuY * accuY);
//
//		// Clamp to a point on controller circle, if we are outside it
//		if (r > kOuterRadius) {
//			accuX = round(accuX * (kOuterRadius - kBounceBack) / r);
//			accuX = round(accuY * (kOuterRadius - kBounceBack) / r);
//			r = sqrt(accuX * accuX + accuY * accuY);
//		}
//
//		float phi = atan2(accuY, accuX);
//
//		auto forStick = [&](auto& conf, auto& ex, short& outX, short& outY) {
//			if (r > conf.sensitivity * kOuterRadius) {
//				float num = r - conf.sensitivity * kOuterRadius;
//				float denom = kOuterRadius - conf.sensitivity * kOuterRadius;
//				SetJoystickPosition(phi, pow(num / denom, conf.nonLinear), conf.invertXAxis, conf.invertYAxis, outX, outY);
//			}
//			else {
//				dev.lstickX = 0;
//				dev.lstickY = 0;
//			}
//			};
//		forStick(profile->lstick.mouse, extra.lstick, dev.lstickX, dev.lstickY);
//		forStick(profile->rstick.mouse, extra.rstick, dev.rstickX, dev.rstickY);
//
//		extra.accuMouseX = 0;
//		extra.accuMouseY = 0;
//	}
//}
//
//static void HandleMouseMovement(HANDLE hDevice, LONG dx, LONG dy, InputTranslationStruct& its) {
//	for (int userIndex = 0; userIndex < 4; ++userIndex) {
//		if (!gXiGamepadsEnabled[userIndex]) continue;
//		HANDLE src = gXiGamepads[userIndex].srcMouse;
//		if (src != INVALID_HANDLE_VALUE && src != hDevice) continue;
//
//		auto& extra = its.sticks[userIndex];
//
//		extra.accuMouseX += dx;
//		extra.accuMouseY += dy;
//	}
//}
//
//static void HandleKeyPress(HANDLE hDevice, BYTE vkey, bool pressed, InputTranslationStruct& its) {
//	for (int userIndex = 0; userIndex < 4; ++userIndex) {
//		if (!gXiGamepadsEnabled[userIndex]) continue;
//		if (IsKeyCodeMouseButton(vkey)) {
//			HANDLE src = gXiGamepads[userIndex].srcMouse;
//			if (src != INVALID_HANDLE_VALUE && src != hDevice) continue;
//		}
//		else {
//			HANDLE src = gXiGamepads[userIndex].srcKbd;
//			if (src != INVALID_HANDLE_VALUE && src != hDevice) continue;
//		}
//
//		auto& dev = gXiGamepads[userIndex];
//		auto& extra = its.sticks[userIndex];
//
//		bool recompute_lstick = false;
//		bool recompute_rstick = false;
//
//		switch (its.btns[userIndex][vkey]) {
//			using enum XiButton;
//		case A: dev.a = pressed; break;
//		case B: dev.b = pressed; break;
//		case X: dev.x = pressed; break;
//		case Y: dev.y = pressed; break;
//
//		case LB: dev.lb = pressed; break;
//		case RB: dev.rb = pressed; break;
//		case LT: dev.lt = pressed; break;
//		case RT: dev.rt = pressed; break;
//
//		case Start: dev.start = pressed; break;
//		case Back: dev.back = pressed; break;
//
//		case DpadUp: dev.dpadUp = pressed; break;
//		case DpadDown: dev.dpadDown = pressed; break;
//		case DpadLeft: dev.dpadLeft = pressed; break;
//		case DpadRight: dev.dpadRight = pressed; break;
//
//		case LStickBtn: dev.lstickBtn = pressed; break;
//		case RStickBtn: dev.rstickBtn = pressed; break;
//
//			// NOTE: we assume that if any key is setup for the joystick directions, it's on keyboard mode
//			//       that is, we rely on the translation struct being populated from the current user config correctly
//#define STICKBUTTON(THEENUM, STICK, DIR) case THEENUM: recompute_##STICK = true; extra.STICK.DIR = pressed; break;
//			STICKBUTTON(LStickUp, lstick, up);
//			STICKBUTTON(LStickDown, lstick, down);
//			STICKBUTTON(LStickLeft, lstick, left);
//			STICKBUTTON(LStickRight, lstick, right);
//			STICKBUTTON(RStickUp, rstick, up);
//			STICKBUTTON(RStickDown, rstick, down);
//			STICKBUTTON(RStickLeft, rstick, left);
//			STICKBUTTON(RStickRight, rstick, right);
//#undef STICKBUTTON
//
//		case None: break;
//		}
//
//		constexpr int kStickMaxVal = 32767;
//		if (recompute_lstick) {
//			// Stick's actual value per user's speed setting
//			int val = (int)(kStickMaxVal * dev.profile->lstick.kbd.speed);
//			dev.lstickX = (extra.lstick.right ? val : 0) + (extra.lstick.left ? -val : 0);
//			dev.lstickY = (extra.lstick.up ? val : 0) + (extra.lstick.down ? -val : 0);
//		}
//		if (recompute_rstick) {
//			int val = (int)(kStickMaxVal * dev.profile->rstick.kbd.speed);
//			dev.rstickX = (extra.rstick.right ? val : 0) + (extra.rstick.left ? -val : 0);
//			dev.rstickY = (extra.rstick.up ? val : 0) + (extra.rstick.down ? -val : 0);
//		}
//
//		++dev.epoch;
//	}
//}
//
//static bool HandleHotkeys(USHORT vkey, AppState& s) {
//	if (vkey == gConfig.hotkeyShowUI) {
//		ShowWindow(s.mainWindow, SW_SHOWNORMAL);
//		SetFocus(s.mainWindow);
//		s.blockingMessagePump = false;
//
//		return true;
//	}
//	else if (vkey == gConfig.hotkeyCaptureCursor) {
//		if (auto hostHwnd = s.mainUI->mainHostHwnd) {
//			if (s.capturingCursor) {
//				ClipCursor(nullptr);
//				ShowCursor(true);
//
//				s.capturingCursor = false;
//				LOG_DEBUG(L"Released cursor");
//			}
//			else {
//				// Get window-space rect of client area
//				RECT clientRect;
//				GetClientRect(hostHwnd, &clientRect);
//
//				POINT tl;
//				tl.x = clientRect.left;
//				tl.y = clientRect.top;
//				POINT br;
//				br.x = clientRect.right;
//				br.y = clientRect.bottom;
//
//				// Map window-space rect to screen-space
//				MapWindowPoints(hostHwnd, nullptr, &tl, 1);
//				MapWindowPoints(hostHwnd, nullptr, &br, 1);
//
//				RECT rect;
//				rect.left = tl.x;
//				rect.top = tl.y;
//				rect.right = br.x;
//				rect.bottom = br.y;
//				ClipCursor(&rect);
//				ShowCursor(false);
//
//				s.capturingCursor = true;
//				LOG_DEBUG(L"Captured cursor");
//			}
//		}
//		else {
//			LOG_DEBUG(L"Main game window not selected, cannot capture cursor");
//		}
//
//		return true;
//	}
//	return false;
//}

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
		if (wParam == SIZE_MINIMIZED)
			return 0;

		auto app = reinterpret_cast<AppState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
		auto resizeWidth = static_cast<UINT>(LOWORD(lParam));
		auto resizeHeight = static_cast<UINT>(HIWORD(lParam));
		app->mainWindow.ResizeRenderTarget(resizeWidth, resizeHeight);
		return 0;
	}

	case WM_CLOSE: {
		auto app = reinterpret_cast<AppState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
		ShowWindow(hWnd, SW_HIDE);
		--app->shownWindowCount;
		return 0;
	}

	case WM_INPUT: {
		//HRAWINPUT hri = (HRAWINPUT)lParam;

		//UINT size = 0;
		//GetRawInputData(hri, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
		//if (size > s.rawinputSize || s.rawinput == nullptr) {
		//	s.rawinput = std::make_unique<std::byte[]>(size);
		//	s.rawinputSize = size;
		//}

		//if (GetRawInputData(hri, RID_INPUT, s.rawinput.get(), &size, sizeof(RAWINPUTHEADER)) == (UINT)-1) {
		//	LOG_DEBUG(L"GetRawInputData() failed");
		//	break;
		//}
		//RAWINPUT* ri = (RAWINPUT*)s.rawinput.get();

		//// We are going to modify/push data onto XiGamepad's below, from this input event
		//SrwExclusiveLock lock(gXiGamepadsLock);

		//switch (ri->header.dwType) {
		//case RIM_TYPEMOUSE: {
		//	const auto& mouse = ri->data.mouse;

		//	// If any button is pressed...
		//	if (mouse.usButtonFlags != 0) {
		//		if (s.uiState->bindIdevFromNextMouse != -1) {
		//			gXiGamepads[s.uiState->bindIdevFromNextMouse].srcMouse = ri->header.hDevice;
		//			s.uiState->bindIdevFromNextMouse = -1;
		//			break;
		//		}
		//	}

		//	if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) HandleKeyPress(ri->header.hDevice, VK_LBUTTON, true, s.its);
		//	if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) HandleKeyPress(ri->header.hDevice, VK_LBUTTON, false, s.its);
		//	if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) HandleKeyPress(ri->header.hDevice, VK_RBUTTON, true, s.its);
		//	if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) HandleKeyPress(ri->header.hDevice, VK_RBUTTON, false, s.its);
		//	if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) HandleKeyPress(ri->header.hDevice, VK_MBUTTON, true, s.its);
		//	if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP) HandleKeyPress(ri->header.hDevice, VK_MBUTTON, false, s.its);
		//	if (mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) HandleKeyPress(ri->header.hDevice, VK_XBUTTON1, true, s.its);
		//	if (mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP) HandleKeyPress(ri->header.hDevice, VK_XBUTTON1, false, s.its);
		//	if (mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) HandleKeyPress(ri->header.hDevice, VK_XBUTTON2, true, s.its);
		//	if (mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP) HandleKeyPress(ri->header.hDevice, VK_XBUTTON2, false, s.its);

		//	if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
		//		LOG_DEBUG("Warning: RAWINPUT reported absolute mouse corrdinates, not supported");
		//		break;
		//	} // else: MOUSE_MOVE_RELATIVE

		//	HandleMouseMovement(ri->header.hDevice, mouse.lLastX, mouse.lLastY, s.its);
		//} break;

		//case RIM_TYPEKEYBOARD: {
		//	const auto& kbd = ri->data.keyboard;

		//	// This message is a part of a longer makecode sequence -- the actual Vkey is in another one
		//	if (kbd.VKey == 0xFF)
		//		break;
		//	// All of the relevant keys that we support fit in a BYTE
		//	if (kbd.VKey > 0xFF)
		//		break;

		//	bool press = !(kbd.Flags & RI_KEY_BREAK);

		//	// If any key is pressed...
		//	if (press) {
		//		if (HandleHotkeys(kbd.VKey, s))
		//			break;

		//		if (s.uiState->bindIdevFromNextKey != -1) {
		//			gXiGamepads[s.uiState->bindIdevFromNextKey].srcKbd = ri->header.hDevice;
		//			s.uiState->bindIdevFromNextKey = -1;
		//			break;
		//		}
		//	}

		//	HandleKeyPress(ri->header.hDevice, (BYTE)kbd.VKey, press, s.its);
		//} break;
		//}

		return 0;
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

MainWindow::MainWindow(AppState& app, HINSTANCE hInstance) {
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

ViGEm::ViGEm()
	: hvigem{ vigem_alloc() }
{
	VIGEM_ERROR err;

	err = vigem_connect(hvigem);
	if (err != VIGEM_ERROR_NONE)
		throw std::runtime_error(std::format("Failed to connect ViGEm bus"));
}

ViGEm::~ViGEm() {
	vigem_disconnect(hvigem);
	vigem_free(hvigem);
}

AppState::AppState(HINSTANCE hInstance)
	: hInstance{ hInstance }
	, mainWindow(*this, hInstance)
{
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
	io.IniFilename = "WinXInputFeeder.imgui-state";

	ImGui_ImplWin32_Init(mainWindow.hWnd);
	ImGui_ImplDX11_Init(mainWindow.d3dDevice, mainWindow.d3dDeviceContext);
}

AppState::~AppState() {
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void AppState::MainRenderFrame() {
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

int AppMain(HINSTANCE hInstance, std::span<const std::wstring_view> args) {
	LOG_DEBUG(L"Starting input source window");

	AppState s(hInstance);

	gConfigEvents.onMouseCheckFrequencyChanged += [&](int newFrequency) {
		SetTimer(nullptr, kMouseCheckTimerID, newFrequency, nullptr);
		};
	gConfigEvents.onGamepadBindingChanged += [&](int userIndex, const std::string& profileName, const UserProfile& profile) {
		s.its.PopulateBtnLut(userIndex, profile);
		};
	ReloadConfigFromDesignatedPath();

	LOG_DEBUG(L"Starting working thread's main loop");
	while (true) {
		MSG msg;

		// The blocking message pump
		// We'll block here, until one of the messages changes changes blockingMessagePump to false (i.e. we should be rendering again) ...
		while (s.shownWindowCount == 0 && GetMessageW(&msg, nullptr, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
			if (msg.message == WM_QUIT)
				break;
		}

		// ... in which case the above loop breaks, and we come here (regular polling message pump) to process the rest, and then enter regular main loop doing rendering + polling
		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);

			// WM_QUIT is gaurenteed to only exist when there is nothing else in the message queue, we can safely exit immediately
			if (msg.message == WM_QUIT)
				break;
		}

		s.MainRenderFrame();
	}

	return 0;
}
