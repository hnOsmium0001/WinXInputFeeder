#pragma once

#include "userconfig.hpp"

struct UIState {
    std::unique_ptr<void, void(*)(void*)> p{ nullptr, nullptr };

    /* [Out] */ HWND mainHostHwnd = NULL;
    // If set to a valid gamepad user index, the next key recieved by the input source will be used to set its keyboard filter
    /* [Out] */ int bindIdevFromNextKey = -1;
    // If set to a valid gamepad user index, the next mouse click recieved by the input source will be used to set its mouse filter
    // Note that it has to be a mouse button click, movements do not count (to prevent misinput).
    /* [Out] */ int bindIdevFromNextMouse = -1;
};

void ShowUI(UIState& s);
