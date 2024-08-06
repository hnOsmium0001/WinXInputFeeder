#include "pch.hpp"

#include "dll.hpp"
#include "filter.hpp"
#include "shadowed.h"
#include "utils.hpp"

HMODULE gHModule;

// Definitions for stuff in shadowed.h
static HMODULE xinput_dll;
//Pfn_XInputEnable pfn_XInputEnable = nullptr;
Pfn_XInputGetAudioDeviceIds pfn_XInputGetAudioDeviceIds = nullptr;
Pfn_XInputGetBatteryInformation pfn_XInputGetBatteryInformation = nullptr;
Pfn_XInputGetCapabilities pfn_XInputGetCapabilities = nullptr;
//Pfn_XInputGetDSoundAudioDeviceGuids pfn_XInputGetDSoundAudioDeviceGuids = nullptr;
Pfn_XInputGetKeystroke pfn_XInputGetKeystroke = nullptr;
Pfn_XInputGetState pfn_XInputGetState = nullptr;
Pfn_XInputSetState pfn_XInputSetState = nullptr;

static void InitializeShadowedPfns() {
	// Using LoadLibraryExW() with LOAD_LIBRARY_SEARCH_SYSTEM32 only doesn't seem to work -- it still found our XInput1_4.dll (if our name is indeed this)
	// On 32-bit process, "System32" is automatically redirected to SysWOW64
	// On 64-bit process, "System32" will work in place
	xinput_dll = LoadLibraryW(L"C:/Windows/System32/XInput1_4.dll");
	if (!xinput_dll) {
		LOG_DEBUG(L"Error opening XInput1_4.dll: {}", GetLastErrorStr());
		return;
	}

	//pfn_XInputEnable = (Pfn_XInputEnable)GetProcAddress(xinput_dll, "XInputEnable");
	pfn_XInputGetAudioDeviceIds = (Pfn_XInputGetAudioDeviceIds)GetProcAddress(xinput_dll, "XInputGetAudioDeviceIds");
	pfn_XInputGetBatteryInformation = (Pfn_XInputGetBatteryInformation)GetProcAddress(xinput_dll, "XInputGetBatteryInformation");
	pfn_XInputGetCapabilities = (Pfn_XInputGetCapabilities)GetProcAddress(xinput_dll, "XInputGetCapabilities");
	//pfn_XInputGetDSoundAudioDeviceGuids = (Pfn_XInputGetDSoundAudioDeviceGuids)GetProcAddress(xinput_dll, "XInputGetDSoundAudioDeviceGuids");
	pfn_XInputGetKeystroke = (Pfn_XInputGetKeystroke)GetProcAddress(xinput_dll, "XInputGetKeystroke");
	pfn_XInputGetState = (Pfn_XInputGetState)GetProcAddress(xinput_dll, "XInputGetState");
	pfn_XInputSetState = (Pfn_XInputSetState)GetProcAddress(xinput_dll, "XInputSetState");
}

static INIT_ONCE gDllInitGuard = INIT_ONCE_STATIC_INIT;
static BOOL CALLBACK DllInitHandler(PINIT_ONCE initOnce, PVOID parameter, PVOID* lpContext) {
	InitializeShadowedPfns();
	return TRUE;
}
static void EnsureDllInit() {
	PVOID ctx = nullptr;
	BOOL status = InitOnceExecuteOnce(&gDllInitGuard, DllInitHandler, nullptr, &ctx);
	if (!status) {
		LOG_DEBUG(L"Failed to execute INIT_ONCE");
	}
}

// This function is deprecated, but we still provide it in case the game uses it
XI_API_FUNC void WINAPI XInputEnable(
	BOOL enable
) WIN_NOEXCEPT {
	EnsureDllInit();
}

XI_API_FUNC DWORD WINAPI XInputGetAudioDeviceIds(
	_In_ DWORD dwUserIndex,
	_Out_writes_opt_(*pRenderCount) LPWSTR pRenderDeviceId,
	_Inout_opt_ UINT* pRenderCount,
	_Out_writes_opt_(*pCaptureCount) LPWSTR pCaptureDeviceId,
	_Inout_opt_ UINT* pCaptureCount
) WIN_NOEXCEPT {
	EnsureDllInit();
	return pfn_XInputGetAudioDeviceIds(dwUserIndex, pRenderDeviceId, pRenderCount, pCaptureDeviceId, pCaptureCount);
}

XI_API_FUNC DWORD WINAPI XInputGetBatteryInformation(
	_In_ DWORD dwUserIndex,
	_In_ BYTE devType,
	_Out_ XINPUT_BATTERY_INFORMATION* pBatteryInformation
) WIN_NOEXCEPT {
	EnsureDllInit();
	return pfn_XInputGetBatteryInformation(dwUserIndex, devType, pBatteryInformation);
}

XI_API_FUNC DWORD WINAPI XInputGetCapabilities(
	_In_ DWORD dwUserIndex,
	_In_ DWORD dwFlags,
	_Out_ XINPUT_CAPABILITIES* pCapabilities
) WIN_NOEXCEPT {
	EnsureDllInit();
	return pfn_XInputGetCapabilities(dwUserIndex, dwFlags, pCapabilities);
}

XI_API_FUNC DWORD WINAPI XInputGetKeystroke(
	_In_ DWORD dwUserIndex,
	_Reserved_ DWORD dwReserved,
	_Out_ XINPUT_KEYSTROKE* pKeystroke
) WIN_NOEXCEPT {
	EnsureDllInit();
	return pfn_XInputGetKeystroke(dwUserIndex, dwReserved, pKeystroke);
}

XI_API_FUNC DWORD WINAPI XInputGetState(
	_In_ DWORD dwUserIndex,
	_Out_ XINPUT_STATE* pState
) WIN_NOEXCEPT {
	EnsureDllInit();
	return pfn_XInputGetState(dwUserIndex, pState);
}

XI_API_FUNC DWORD WINAPI XInputSetState(
	_In_ DWORD dwUserIndex,
	_In_ XINPUT_VIBRATION* pVibration
) WIN_NOEXCEPT {
	EnsureDllInit();
	return pfn_XInputSetState(dwUserIndex, pVibration);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) noexcept {
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		gHModule = hModule;
		InstallMessageFilter();

		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
