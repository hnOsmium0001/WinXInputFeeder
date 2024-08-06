#pragma once

#include <minwindef.h>

LRESULT CALLBACK GetMsgProc(int code, WPARAM wParam, LPARAM lParam);

void InstallMessageFilter();
