#pragma once

#include <iostream>
#include <windows.h>

namespace input {
    void enable( HWND window, HWND game_window );
    void disable( HWND window , HWND game_window );
}