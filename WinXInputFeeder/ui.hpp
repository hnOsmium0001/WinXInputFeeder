#pragma once

#include "userconfig.hpp"

struct UIState {
    void* p;

    /* [Out] */ int bindIdevFromNextKey = -1;
    // If set to a valid gamepad user index, the next mouse click recieved by the input source will be used to set its mouse filter
    // Note that it has to be a mouse button click, movements do not count (to prevent misinput).
    /* [Out] */ int bindIdevFromNextMouse = -1;

    UIState();
    ~UIState();

    void Show();
};
