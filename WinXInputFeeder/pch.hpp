#pragma once

////////// Standard library //////////

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <functional>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

////////// System headers //////////

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <d3d11.h>
#include <hidusage.h>
#include <tlhelp32.h>
#include <shellapi.h>

////////// 3rd party headers //////////

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <imgui_stdlib.h>
#include <toml++/toml.h>
