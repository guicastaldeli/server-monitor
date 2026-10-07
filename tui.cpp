#include "tui.hpp"
#include "platform.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>

namespace smon {

char ConvKey(char c) {
    char val = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return val;
}

static std::string padRight(const std::string& s, size_t w) {
    if(s.size() >= w) return s;
    return s + std::string(w - s.size(), ' ');
}

static ColWidths computeWidths(const ProjectView& pv) {
    ColWidths cw;
    size_t ncols = 0;
    for(const auto& r : pv.rows) {
        if(r.isEndpoint) { ncols = r.fields.size(); break; }
    }
    cw.widths.assign(ncols, 0);

    for(const auto& r : pv.rows) {
        if(!r.isEndpoint) continue;
        for(size_t i = 0; i < r.fields.size() && i < ncols; ++i) {
            cw.widths[i] = std::max(cw.widths[i], r.fields[i].label.size());
            cw.widths[i] = std::max(cw.widths[i], r.fields[i].value.size());
        }
        if(r.status.size() > cw.statusWidth) cw.statusWidth = r.status.size();
    }

    return cw;
}

static void refreshRowStatuses(
    std::vector<ProjectView>& views,
    const std::vector<Project>& projects,
    const StatusMonitor& monitor
) { 
    for(size_t pi = 0; pi < views.size(); ++pi) {
        if(pi >= projects.size()) break;
        const auto& proj = projects[pi];
        auto& pv = views[pi];

        size_t ei = 0;
        for(auto& row : pv.rows) {
            if(!row.isEndpoint) continue;
            if(ei >= proj.endpoints.size()) break;
            const auto& ep = proj.endpoints[ei];
            row.status = monitor.statusOf(ep.service, ep.rawUrl);
            ++ei;
        }
    }
}   

static void dispatchAction(const FieldInfo& f) {
    if(f.action) f.action(f.value);
}

/**
 * 
 * Terminal
 * 
 */
#ifdef _WIN32
static HANDLE g_hStdin      = INVALID_HANDLE_VALUE;
static HANDLE g_hStdout      = INVALID_HANDLE_VALUE;
static DWORD g_oldInMode    = 0;
static DWORD g_oldOutMode   = 0;
#else
static termios g_oldTermios;
static bool g_termiosSaved  = false;
#endif;

void terminalInit() {
#ifdef _WIN32
    g_hStdin = GetStdHandle(STD_INPUT_HANDLE);
    g_hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

    GetConsoleMode(g_hStdin, &g_oldInMode);
    GetConsoleMode(g_hStdout, &g_oldOutMode);

    DWORD inMode = g_oldInMode;
    inMode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    inMode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    SetConsoleMode(g_hStdin, inMode);

    DWORD outMode = g_oldOutMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(g_hStdout, outMode);
    SetConsoleOutputCP(CP_UTF8);
#else
    tcgetattr(STDIN_FILENO, &g_oldTermios);
    g_termiosSaved = true;

    termios raw = g_oldTermios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
#endif
}

void terminalRestore() {
#ifdef _WIN32
    if(g_hStdin != INVALID_HANDLE_VALUE) SetConsoleMode(g_hStdin, g_oldInMode);
    if(g_hStdout != INVALID_HANDLE_VALUE) SetConsoleMode(g_hStdout, g_oldOutMode);
#else
    if(g_termiosSaved) tcsetattr(STDIN_FILENO, TCSANOW, &g_oldTermios);
#endif
}

Key readKey() {
#ifdef _WIN32
    int c = _getch();
    char k = ConvKey((char(c)));
    if(c == 0 || c == 0xE0) {
        int c2 = _getch();
        switch(c2) {
            case 38:            return Key::Up;
            case 40:            return Key::Down;
            case 37:            return Key::Left;
            case 39:            return Key::Right;
        }
        return Key::None;
    }
    if(k == 'q')                return Key::Quit;
    if(k == 'r')                return Key::Refresh;
    if(c == 27)                 return Key::Escape;
    if(c == 13 || c == 10)      return Key::Enter;
    return Key::None;
#else
    unsigned char c;
    char k = ConvKey((char)c);
    if(::read(STDIN_FILENO, &c, 1) != 1) return Key::None;
    if(c == 27) {
        unsigned char seq[2];
        
        termios t;
        tcgetattr(STDIN_FILENO, &t);
        termios t2 = t;
        t2.c_cc[VMIN] = 0;
        t2.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &t2);

        int n1 = ::read(STDIN_FILENO, &seq[0], 1);
        if(n1 != 1) {
            tcsetattr(STDIN_FILENO, TCANOW, &t);
            return Key::Escape;
        }
        int n2 = ::read(STDIN_FILENO, &seq[1], 1);
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
        if(n1 == 1 && n2 == 1 && seq[0] == '[') {
            switch(seq[1]) {
                case 'KEY_UP':      return Key::Up;
                case 'KEY_DOWN':    return Key::Down;
                case 'KEY_RIGHT':   return Key::Right;
                case 'KEY_LEFT':    return Key::Left;
            }
        }
        return Key::Escape;
    }
    if(k == 'q')                    return Key::Quit;
    if(k == 'r')                    return Key::Refersh;
    if(c == '\n' || c == '\r')      return Key::Enter;
    return Key::None;
#endif   
}

/**
 * 
 * Rendering...
 * 
 */
void clearScreen() {
    std::cout << "\x1b[2J\x1b[H" << std::flush;
}

void hideCursor() {
    std::cout << "\x1b[?25l" << std::flush;
}

void showCursor() {
    std::cout << "\x1b[?25h" << std::flush;
}

void moveCursor(int row, int col) {
    std::cout << "\x1b[" << row << ";" << col << "H" << std::flush;
}

static void renderAll(
    const std::string& rootFolder,
    const std::vector<ProjectView>& views,
    const CursorState& cur,
    const StatusMonitor& monitor
) {

}

void render(
    const std::string& rootFolder,
    const std::vector<Project>& projects,
    const std::vector<ProjectView>& views,
    StatusMonitor& monitor
) {
    
}

/**
 * 
 * Openers
 * 
 */
bool openInBrowser(const std::string& url) {
#ifdef _WIN32
    HINSTANCE h = ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<intptr_t>(h) > 32;
#else
    pid_t pid = fork();
    if(pid == 0) {
        execlp("xdg-open", "xdg-open", url.c_str(), (char*)nullptr);
        _exit(127);
    }
    return pid > 0;
#endif
}

bool openInFileManager(const std::string& path) {
#ifdef _WIN32
    HINSTANCE h = ShellExecuteA(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<intptr_t>(h) > 32;
#else
    pid_t pid = fork();
    if(pid == 0) {
        execlp("xdg-open", "xdg-open", path.c_str(), (char*)nullptr);
        _exit(127);
    }
    return pid > 0;
#endif
}

}