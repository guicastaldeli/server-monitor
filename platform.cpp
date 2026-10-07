#include "platform.hpp"

namespace smon {

bool socketInit() {
#ifdef _WIN32
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
#else
    return true;
#endif
}

void socketCleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}

void closeSocket(socket_t s) {
    if (s == INVALID_SOCK) return;
#ifdef _WIN32
    closesocket(s);
#else
    ::close(s);
#endif
}

bool setNonblocking(socket_t s, bool nonblocking) {
#ifdef _WIN32
    u_long mode = nonblocking ? 1 : 0;
    return ioctlsocket(s, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0) return false;
    if (nonblocking) flags |= O_NONBLOCK;
    else             flags &= ~O_NONBLOCK;
    return fcntl(s, F_SETFL, flags) == 0;
#endif
}

}