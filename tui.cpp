#include "tui.hpp"
#include "platform.hpp"
#include "color.hpp"
#include "status.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <chrono>

namespace smon {

char ConvKey(char c) {
    char val = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return val;
}

static std::string Repeat(const std::string& s, size_t count) {
    std::string out;
    out.reserve(s.size() * count);
    for(size_t i = 0; i < count; i++) out.append(s);
    return out;
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

        size_t labelWidth = std::string(infoOf(r.status).label).size();
        if(labelWidth > cw.statusWidth) cw.statusWidth = labelWidth;
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
#endif

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
    if(!_kbhit()) return Key::None;

    int c = _getch();
    char k = ConvKey((char)c);
    if(c == 0 || c == 0xE0) {
        int c2 = _getch();
        switch(c2) {
            case 72:            return Key::Up;
            case 80:            return Key::Down;
            case 75:            return Key::Left;
            case 77:            return Key::Right;
        }
        return Key::None;
    }
    if(k == 'q')                return Key::Quit;
    if(k == 'r')                return Key::Refresh;
    if(c == 27)                 return Key::Escape;
    if(c == 13 || c == 10)      return Key::Enter;
    return Key::None;
#else
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;

    int sel = ::select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv);
    if(sel <= 0) return Key::None;

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
            tcsetattr(STDIN_FILENO, TCSANOW, &t);
            return Key::Escape;
        }
        int n2 = ::read(STDIN_FILENO, &seq[1], 1);
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
        if(n1 == 1 && n2 == 1 && seq[0] == '[') {
            switch(seq[1]) {
                case 'A':           return Key::Up;
                case 'B':           return Key::Down;
                case 'C':           return Key::Right;
                case 'D':           return Key::Left;
            }
        }
        return Key::Escape;
    }
    if(k == 'q')                    return Key::Quit;
    if(k == 'r')                    return Key::Refresh;
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
    std::ostringstream out;

    out << Repeat("=", 50) << "\n";
    out << "\n";
    out << Repeat(" ", 17) << "Server Monitor\n";
    out << "\n";
    out << Repeat("=", 50) << "\n";

    out << "\n";
    out << "[Arrows] Navigate   [Enter] Enter/Open   [Esc] Back/Quit   [R] Refresh   [Q] Quit\n";
    out << "\n";

    out << "--- Folder: " << rootFolder << "\n\n";
    out << "PROJECTS:\n\n";

    for(size_t pi = 0; pi < views.size(); ++pi) {
        const auto& pv = views[pi];
        
        // Outside project-level
        if(cur.mode == Mode::SelectProject && (int)pi == cur.projectIdx) {
            out << CURSOR_CHAR << pv.name << "\n";
        } else if(cur.mode == Mode::InsideProject && (int)pi == cur.projectIdx) {
            out << "** " << pv.name << "\n";
        } else {
            out << "   " << pv.name << "\n";
        }

        // Project-level fields
        for(size_t r = 0; r < pv.rows.size(); ++r) {
            const auto& row = pv.rows[r];
            if(row.isEndpoint) break;
            for(size_t c = 0; c < row.fields.size(); ++c) {
                bool sel = (cur.mode == Mode::InsideProject) &&
                            ((int)pi == cur.projectIdx) &&
                            ((int)r == cur.row) &&
                            ((int)c == cur.col);
                out << (sel ? "   > " : "     ")
                    << row.fields[c].label << ": "
                    << row.fields[c].value << "\n";
            }
        }

        out << "     " << Repeat("-", 20) << "z\n";
        
        bool hasEndpoints = false;
        for(const  auto& r : pv.rows) if(r.isEndpoint) { hasEndpoints = true; break; }
        if(hasEndpoints) {
            ColWidths cw = computeWidths(pv);

            // header row
            for(const auto& r : pv.rows) {
                if(!r.isEndpoint) continue;
                out << "  ";
                for(size_t c = 0; c < r.fields.size(); ++c) {
                    out << padRight(r.fields[c].label, cw.widths[c]);
                    if(c + 1 < r.fields.size()) out << "  |  ";
                }
                out << "  |  " << padRight("STATUS", cw.statusWidth) << "\n";

                break;
            }
            for(size_t r = 0; r < pv.rows.size(); ++r) {
                const auto& row = pv.rows[r];
                if(!row.isEndpoint) continue;

                bool rowSelected = (cur.mode == Mode::InsideProject) &&
                                    ((int)pi == cur.projectIdx) &&
                                    ((int)r == cur.row);
                out << "     ";
                for(size_t c = 0; c < row.fields.size(); ++c) {
                    bool cellSel = rowSelected && ((int)c == cur.col);
                    bool hovered = cellSel && row.fields[c].isUrl;
                    std::string cellStyle;

                    if(hovered) {
                    #ifdef _WIN32
                        if(row.status == Status::Online) cellStyle = styleCode("blink");
                        if(row.status == Status::Offline) cellStyle = styleCode("dim");
                    #else
                        if(row.status == Status::Online) if(blinkOn()) cellStyle = styleCode("dim");
                        if(row.status == Status::Offline) cellStyle = styleCode("dim");
                    #endif
                    }

                    std::string cell = row.fields[c].value;
                    out << (cellSel ? CURSOR_CHAR : "  ");
                    out << cellStyle;
                    out << padRight(cell, cw.widths[c]);
                    if(!cellStyle.empty()) out << ANSI_RESET;
                    if(c + 1 < row.fields.size()) out << "  |  ";
                }

                const auto& info = infoOf(row.status);
                std::string stylePrefix;
                bool wblink = false;
                for(const auto& s : info.styles) {
                    if(s == "blink") { wblink = true; continue; }
                    stylePrefix += styleCode(s);
                }
                if(wblink && blinkOn()) {
                    stylePrefix += styleCode("dim");
                }

                std::string colorStart = hexToAnsiFg(info.color);
                out << "  |  " << stylePrefix << colorStart
                    << padRight(info.label, cw.statusWidth)
                    << ANSI_RESET << "\n";
            }
        }

        out << "\n";
    }

    clearScreen();
    std::cout << out.str() << std::flush;
}

void render(
    const std::string& rootFolder,
    const std::vector<Project>& projects,
    const std::vector<ProjectView>& viewsIn,
    StatusMonitor& monitor
) {
    std::vector<ProjectView> views = viewsIn;
    CursorState cur;

    terminalInit();
    hideCursor();

    bool running = true;
    while(running) {
        refreshRowStatuses(views, projects, monitor);
        renderAll(rootFolder, views, cur, monitor);

        Key k = readKey();
        if(k == Key::None) continue;
        
        switch(k) {
            // Quit
            case Key::Quit:
                running = false;
                break;
            // Refresh
            case Key::Refresh:
                monitor.requestRefresh();
                break;
            // Up
            case Key::Up: {
                if(cur.mode == Mode::SelectProject) {
                    if(cur.projectIdx > 0) --cur.projectIdx;
                } else {
                    if(cur.row > 0) {
                        --cur.row;
                        const auto& pv = views[cur.projectIdx];
                        int maxc = (int)pv.rows[cur.row].fields.size() - 1;
                        if(maxc < 0) maxc = 0;
                        if(cur.col > maxc) cur.col = maxc;
                    }
                }
            } break;
            // Down
            case Key::Down: {
                if(cur.mode == Mode::SelectProject) {
                    if(cur.projectIdx + 1 < (int)views.size()) ++cur.projectIdx;
                } else {
                    const auto& pv = views[cur.projectIdx];
                    if(cur.row + 1 < (int)pv.rows.size()) {
                        ++cur.row;
                        int maxc = (int)pv.rows[cur.row].fields.size() - 1;
                        if(maxc < 0) maxc = 0;
                        if(cur.col > maxc) cur.col = maxc;
                    }
                }
            } break;
            // Left
            case Key::Left: {
                if(cur.mode == Mode::InsideProject) {
                    if(cur.col > 0) --cur.col;
                }
            } break;
            // Right
            case Key::Right: {
                if(cur.mode == Mode::InsideProject) {
                    const auto& pv = views[cur.projectIdx];
                    int maxc = (int)pv.rows[cur.row].fields.size() - 1;
                    if(maxc < 0) maxc = 0;
                    if(cur.col < maxc) ++cur.col;
                }
            } break;
            // Enter
            case Key::Enter: {
                if(cur.mode == Mode::SelectProject) {
                    if(!views.empty()) {
                        cur.mode = Mode::InsideProject;
                        cur.row = 0;
                        cur.col = 0;
                    }
                } else {
                    const auto& pv = views[cur.projectIdx];
                    if(cur.row >= 0 && cur.row < (int)pv.rows.size()) {
                        const auto& row = pv.rows[cur.row];
                        if(cur.col >= 0 && cur.col < (int)row.fields.size()) {
                            dispatchAction(row.fields[cur.col]);
                        }
                    }
                }
            } break;
            // ESC
            case Key::Escape: {
                if(cur.mode == Mode::InsideProject) {
                    cur.mode = Mode::SelectProject;
                } else {
                    running = false;
                }
            } break;

            default:
                break;
        }
    }

    showCursor();
    terminalRestore();
    clearScreen();
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