#include "pch.hpp"

#include "dll.hpp"
#include "filter.hpp"

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

static HHOOK gInstalledHook;

void InstallMessageFilter() {
	gInstalledHook = SetWindowsHookExW(WH_GETMESSAGE, GetMsgProc, nullptr, GetCurrentThreadId());
}
