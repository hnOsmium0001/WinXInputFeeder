#include "pch.hpp"

#include "dll.hpp"
#include "filter.hpp"
#include "utils.hpp"

HMODULE gHModule;

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
