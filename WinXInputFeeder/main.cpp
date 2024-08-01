#include "pch.hpp"

#include "inputdevice.hpp"

#include <memory>
#include <shellapi.h>

// Sync declaration with the one in inputsrc.hpp
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
	
	//try {
		retval = AppMain(hInstance, std::span(args.get(), argCount));
	//}
	//catch (const std::exception& err) {
	//	fprintf(stderr, "Uncaught exception: %s\n", err.what());
	//	retval = 255;
	//}

	LocalFree(argList);

	return retval;
}
