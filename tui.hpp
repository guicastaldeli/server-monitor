#pragma once

#include <iostream>
#include <cstdlib>

#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
    #include <shellapi.h>
#else
    #include <termios.h>
    #include <unistd.h>
    #include <cstdio>
#endif

namespace smon {

char ConvKey(char c);

// Terminal
enum class Key { None, 
                Up, Down, Left, Right,
                Enter, Escape, 
                Quit, Refresh };

void terminalInit();
void terminalRestore();
Key readKey();

// Rendering...
void clearScreen();
void hideCursor();
void showCursor();
void moveCursor(int row, int col);

// External openers
bool openInBrowser(const std::string& url);
bool openInFileManager(const std::string& path);

}