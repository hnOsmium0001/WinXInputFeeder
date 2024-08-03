#include "pch.hpp"

#include "inputdevice.hpp"
#include "utils.hpp"

#include <memory>
#include <shellapi.h>

// Sync declaration with the one in app.hpp
// We declare separately instead of just including to avoid recompiling this file every time that header is changed
int AppMain(HINSTANCE hInstance, std::span<const std::wstring_view> args);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	InitKeyCodeConv();

	int argCount;
	PWSTR* argList = CommandLineToArgvW(pCmdLine, &argCount);
	
	auto args = std::make_unique<std::wstring_view[]>(argCount);
	for (int i = 0; i < argCount; ++i) {
		args[i] = std::wstring_view(argList[i]);
	}

	int retval = 0;
	
#ifdef _DEBUG
	retval = AppMain(hInstance, std::span(args.get(), argCount));
#else
	try {
		retval = AppMain(hInstance, std::span(args.get(), argCount));
	}
	catch (const std::exception& err) {
		MessageBoxW(nullptr, Utf8ToWide(err.what()).c_str(), L"Fatal Error", MB_OK | MB_ICONERROR);
		retval = 255;
	}
#endif

	LocalFree(argList);

	return retval;
}
