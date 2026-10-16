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
#ifndef _WIN32
    #include <sys/ioctl.h>
#endif

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

static int getTerminalRows() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
    struct winsize ws;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) return ws.ws_row;
    return 24;
#endif
}

static int getTerminalCols() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
    struct winsize ws;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) return ws.ws_col;
    return 80;
#endif
}

static int visibleLength(const std::string& s) {
    int count = 0;
    bool inEscape = false;
    for(char c : s) {
        if(inEscape) {
            if(c == 'm') inEscape = false;
            continue;
        }
        if(c == '\x1b') {
            inEscape = true;
            continue;
        }
        ++count;
    }

    return count;
}

static std::string truncateVisible(const std::string& s, int maxWidth) {
    if(visibleLength(s) <= maxWidth) return s;

    std::string out;
    int count = 0;
    bool inEscape = false;
    bool hadStyle = false;
    for(char c : s) {
        if(inEscape) {
            out += c;
            if(c == 'm') inEscape = false;
            continue;
        }
        if(c == '\x1b') {
            inEscape = true;
            hadStyle = true;
            out += c;
            continue;
        }
        if(count >= maxWidth) break;
        out += c;
        ++count;
    }
    if(hadStyle) out += ANSI_RESET;
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

    std::cout << "\x1b[?1000h\x1b[?1006h" << std::flush;
#endif
}

void terminalRestore() {
#ifdef _WIN32
    if(g_hStdin != INVALID_HANDLE_VALUE) SetConsoleMode(g_hStdin, g_oldInMode);
    if(g_hStdout != INVALID_HANDLE_VALUE) SetConsoleMode(g_hStdout, g_oldOutMode);
#else
    std::cout << "\x1b[?1000l\x1b[?1006l" << std::flush;
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
    if(c == 32)                 return Key::Space;
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
    if(::read(STDIN_FILENO, &c, 1) != 1) return Key::None;
    if(c == 27) {
        unsigned char seq[16];
        int n = 0;
        
        termios t;
        tcgetattr(STDIN_FILENO, &t);
        termios t2 = t;
        t2.c_cc[VMIN] = 0;
        t2.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &t2);

        while(n < 15) {
            unsigned char ch;
            if(::read(STDIN_FILENO, &ch, 1) != 1) break;
            seq[n++] = ch;
        }
        tcsetattr(STDIN_FILENO, TCSANOW, &t);

        if(n == 0) return Key::Escape;

        if(n >= 3 && seq[0] == '[' && seq[1] == '<') {
            int button = 0, col = 0, row = 0;
            int i = 2;
            while(i < n && seq[i] >= '0' && seq[i] <= '9') { button = button*10 + (seq[i]-'0'); ++i; }
            if(i < n && seq[i] == ';') ++i;
            while(i < n && seq[i] >= '0' && seq[i] <= '9') { col = col*10 + (seq[i]-'0'); ++i; }
            if(i < n && seq[i] == ';') ++i;
            while(i < n && seq[i] >= '0' && seq[i] <= '9') { row = row*10 + (seq[i]-'0'); ++i; }
        
            if(button == 64)        return Key::ScrollUp;
            if(button == 65)        return Key::ScrollDown;
        }

        if(n >= 2 && seq[0] == '[') {
            switch(seq[1]) {
                case 'A':           return Key::Up;
                case 'B':           return Key::Down;
                case 'C':           return Key::Right;
                case 'D':           return Key::Left;
            }
        }

        return Key::Escape;
    }
    char k = ConvKey((char)c);
    if(k == 'q')                    return Key::Quit;
    if(k == 'r')                    return Key::Refresh;
    if(c == '\n' || c == '\r')      return Key::Enter;
    if(c == ' ')                    return Key::Space;
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

void clear() {
    std::cout << "\x1b[?1049h" << std::flush;
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

static std::string build(
    const std::string& rootFolder,
    const std::vector<ProjectView>& views,
    const CursorState& cur,
    const StatusMonitor& monitor
) {
    std::ostringstream out;

    int maxRows = getTerminalRows() - 1;
    if(maxRows < 5) maxRows = 5;
    int written = 0;

    auto put = [&](const std::string& s) {
        out << s;
        for(char ch : s) if(ch == '\n') ++written;
    };

    put(hexToAnsiFg("#dfb3f4") + Repeat("=", 60) + ANSI_RESET + "\n");
    put("\n");
    put(Repeat(" ", 24) + hexToAnsiFg("#c36eed") + "Server Monitor" + ANSI_RESET + "\n");
    put("\n");
    put(hexToAnsiFg("#dfb3f4") + Repeat("=", 60) + ANSI_RESET + "\n");
    put("\n");
    put(hexToAnsiFg("#165cd6")
        + "[Arrows] Navigate   [Enter] Open   [Space] Logs   [Esc] Back/Quit   [R] Refresh   [Q] Quit"
        + ANSI_RESET + "\n");
    put("\n");
    put(hexToAnsiFg("#d6cc16") + "--- Folder: " + rootFolder + ANSI_RESET + "\n\n");
    put("PROJECTS:\n\n");

    int headerLines = written;

    for(size_t pi = 0; pi < views.size(); ++pi) {
        const auto& pv = views[pi];

        std::string nameColor;
        if(cur.mode == Mode::InsideProject && (int)pi == cur.projectIdx) {
            nameColor = styleCode("bold") + hexToAnsiFg("#00ff00");
        } else {
            nameColor = hexToAnsiFg("#009e00");
        }

        if(cur.mode == Mode::SelectProject && (int)pi == cur.projectIdx) {
            put(CURSOR_CHAR + nameColor + pv.name + ANSI_RESET + "\n");
        } else if(cur.mode == Mode::InsideProject && (int)pi == cur.projectIdx) {
            put("** " + nameColor + pv.name + ANSI_RESET + "\n");
        } else {
            put("   " + nameColor + pv.name + ANSI_RESET + "\n");
        }

        for(size_t r = 0; r < pv.rows.size(); ++r) {
            const auto& row = pv.rows[r];
            if(row.isEndpoint) break;
            for(size_t c = 0; c < row.fields.size(); ++c) {
                bool sel = (cur.mode == Mode::InsideProject) &&
                            ((int)pi == cur.projectIdx) &&
                            ((int)r == cur.row) &&
                            ((int)c == cur.col);
                put(std::string(sel ? "   > " : "     ")
                    + row.fields[c].label + ": "
                    + row.fields[c].value + "\n");
            }
        }

        put("       " + Repeat("-", 60) + "\n");

        bool hasEndpoints = false;
        for(const auto& r : pv.rows) if(r.isEndpoint) { hasEndpoints = true; break; }
        if(hasEndpoints) {
            ColWidths cw = computeWidths(pv);

            for(const auto& r : pv.rows) {
                if(!r.isEndpoint) continue;
                std::string headerLine = "     ";
                for(size_t c = 0; c < r.fields.size(); ++c) {
                    headerLine += "  ";
                    headerLine += padRight(r.fields[c].label, cw.widths[c]);
                    if(c + 1 < r.fields.size()) headerLine += "  |  ";
                }
                headerLine += "  |  " + padRight("STATUS", cw.statusWidth) + "\n";
                put(headerLine);
                break;
            }

            for(size_t r = 0; r < pv.rows.size(); ++r) {
                const auto& row = pv.rows[r];
                if(!row.isEndpoint) continue;

                bool rowSelected = (cur.mode == Mode::InsideProject) &&
                                    ((int)pi == cur.projectIdx) &&
                                    ((int)r == cur.row);
                std::string rowLine = "     ";
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
                    rowLine += (cellSel ? CURSOR_CHAR : "  ");
                    rowLine += cellStyle;
                    rowLine += padRight(cell, cw.widths[c]);
                    if(!cellStyle.empty()) rowLine += ANSI_RESET;
                    if(c + 1 < row.fields.size()) rowLine += "  |  ";
                }

                const auto& info = infoOf(row.status);
                std::string stylePrefix;
                bool wblink = false;
                for(const auto& s : info.styles) {
                    if(s == "blink") { wblink = true; continue; }
                    stylePrefix += styleCode(s);
                }

                RGB baseRgb = parseHex(info.color);
                if(wblink && blinkOn()) {
                #ifdef _WIN32
                    stylePrefix += styleCode("dim");
                #else
                    darkenColor(baseRgb);
                #endif
                }

                std::string colorStart = toAnsiFg(baseRgb);
                rowLine += "  |  " + stylePrefix + colorStart
                    + padRight(info.label, cw.statusWidth)
                    + ANSI_RESET + "\n";
                put(rowLine);
            }
        }

        put("\n");
    }

    std::string full = out.str();
    std::vector<std::string> lines;
    {
        std::stringstream ss(full);
        std::string ln;
        while(std::getline(ss, ln)) lines.push_back(ln + "\n");
    }

    int cols = getTerminalCols();
    if(cols < 20) cols = 80;

    for(auto& ln : lines) {
        bool hasNL = !ln.empty() && ln.back() == '\n';
        std::string s = hasNL ? ln.substr(0, ln.size() - 1) : ln;
        s = truncateVisible(s, cols - 1);
        ln = s + (hasNL ? "\n" : "");
    }

    std::ostringstream sliced;
    for(int i = 0; i < headerLines && i < (int)lines.size(); ++i) sliced << lines[i];

    int bodyStart = cur.scrollOffset;
    int bodyEnd = bodyStart + (maxRows - headerLines);
    if(bodyEnd > (int)lines.size()) bodyEnd = (int)lines.size();

    for(int i = bodyStart; i < bodyEnd; ++i) sliced << lines[i];

    return sliced.str();
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

    clear();

    std::string lastFrame;
    bool running = true;
    while(running) {
        refreshRowStatuses(views, projects, monitor);
        std::string frame = build(rootFolder, views, cur, monitor);
        if(frame != lastFrame) {
            std::cout << "\x1b[H\x1b[J" << frame << std::flush;
            lastFrame = frame;
        }

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

                if(cur.scrollOffset > 0) --cur.scrollOffset;
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

                int bodyRows = getTerminalRows() - 12;
                if(bodyRows < 5) bodyRows = 5;
                if(cur.row >= bodyRows) ++cur.scrollOffset;
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
            // Space
            case Key::Space: {
                if(cur.mode != Mode::InsideProject) break;

                const auto& pv = views[cur.projectIdx];
                if(cur.row < 0 || cur.row >= (int)pv.rows.size()) break;

                const auto& row = pv.rows[cur.row];
                if(!row.isEndpoint) break;

                if(cur.projectIdx >= (int)projects.size()) break;
                const auto& proj = projects[cur.projectIdx];
                if(row.sourceIndex < 0 || row.sourceIndex >= (int)proj.endpoints.size()) break;
            
                const auto& ep = proj.endpoints[row.sourceIndex];

                terminalRestore();
                showCursor();

                std::cout << "\n--- docker compose logs -f " << ep.service << " (Ctrl+C to exit) ---\n\n" << std::flush;

                openComposeLogs(proj.path, proj.composePath, ep.service);

                std::cout << "\n--- Back to Server Monitor ---\n" << std::flush;
                terminalInit();
                hideCursor();
                lastFrame.clear();
            }
            // Scroll Up
            case Key::ScrollUp: {
                int limit = 0;
                if(cur.scrollOffset > limit) --cur.scrollOffset;
            } break;
            // Scroll Down
            case Key::ScrollDown: {
                int limit = 100;
                if(cur.scrollOffset < limit) ++cur.scrollOffset;
            } break;

            default:
                break;
        }
    }

    clear();
    showCursor();
    terminalRestore();
    clearScreen();
}

/**
 * 
 * Openers
 * 
 */
bool openComposeLogs(
    const std::string& projectPath,
    const std::string& composePath,
    const std::string& service
) {
    if(service.empty()) return false;

    terminalRestore();

    std::string cmd;
#ifdef _WIN32
    cmd = "cmd /c \"docker compose -f \"" + composePath + "\" logs -f " + service + "\"";
#else
    cmd = "docker compose -f '" + composePath + "' logs -f " + service;
#endif

    int rc = std::system(cmd.c_str());

    terminalInit();

    return rc == 0;
}

/**
 * 
 * External Openers
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