#pragma once

#include "view.hpp"
#include "endpoint.hpp"
#include "status_monitor.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>
#include <chrono>

#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
    #include <shellapi.h>
#else
    #include <termios.h>
    #include <unistd.h>
    #include <cstdio>
    #include <sys/select.h>
#endif

#define CURSOR_CHAR "> "

namespace smon {

enum class Mode {
    SelectProject,
    InsideProject
};

struct CursorState {
    Mode mode = Mode::SelectProject;
    int projectIdx = 0;
    int row = 0;
    int col = 0;
};

struct ColWidths {
    std::vector<size_t> widths;
    size_t statusWidth = 6;
};

char ConvKey(char c);
static std::string Repeat(const std::string& s, size_t count);

static std::string padRight(const std::string& s, size_t w);
static ColWidths computeWidths(const ProjectView& pv);
static void refreshRowStatuses(std::vector<ProjectView>& views,
                                const std::vector<Project>& projects,
                                const StatusMonitor& monitor);

static void dispatchAction(const FieldInfo& f);

/**
 * 
 * Terminal
 * 
 */
enum class Key { None, 
                Up, Down, Left, Right,
                Enter, Escape, 
                Quit, Refresh };

void terminalInit();
void terminalRestore();
Key readKey();

/**
 * 
 * Rendering...
 * 
 */
void clearScreen();
void clear();
void hideCursor();
void showCursor();
void moveCursor(int row, int col);
void render(const std::string& rootFolder,
            const std::vector<Project>& projects,
            const std::vector<ProjectView>& viewsIn,
            StatusMonitor& monitor);
static void renderAll(const std::string& rootFolder,
                        const std::vector<ProjectView>& views,
                        const CursorState& cur,
                        const StatusMonitor& monitor);

}