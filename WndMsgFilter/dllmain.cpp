#include "dll.hpp"
#include "utils.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

HMODULE gHModule;

LRESULT CALLBACK GetMsgProc(int code, WPARAM wParam, LPARAM lParam) {
	if (code < 0)
		return CallNextHookEx(nullptr, code, wParam, lParam);

	if (code == HC_ACTION) {
		auto msg = reinterpret_cast<MSG*>(lParam);
		switch (msg->message) {
		case WM_KEYDOWN:
		case WM_KEYUP:
			// Remove the message
			msg->message = 0;
			return 0;

		default:
			break;
		}
	}

	return CallNextHookEx(nullptr, code, wParam, lParam);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) noexcept {
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		gHModule = hModule;
		gInstalledHook = SetWindowsHookExW(WH_GETMESSAGE, GetMsgProc, nullptr, GetCurrentThreadId());

		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
