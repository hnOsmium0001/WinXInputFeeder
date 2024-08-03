#pragma once

#include "modelconfig.hpp"

class App;
class FeederEngine;

class UIState {
private:
	void* p;

public:
	/* [Out] */ int bindIdevFromNextKey = -1;
	// If set to a valid gamepad user index, the next mouse click recieved by the input source will be used to set its mouse filter
	// Note that it has to be a mouse button click, movements do not count (to prevent misinput).
	/* [Out] */ int bindIdevFromNextMouse = -1;

public:
	UIState(App&);
	~UIState();

	void OnFeederEngine(FeederEngine*);

	void Show();
};
