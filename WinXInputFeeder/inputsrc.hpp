#pragma once

#include <string_view>
#include <span>

// Sync declaration with the one in main.cpp
int AppMain(HINSTANCE hInstance, std::span<const std::wstring_view> args);
