#pragma once
// Cross platform socket initialization + connect helpers

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "win2_32.lib")
    using socket_t = SOCKET;
    static constexpr socket_t INVALID_SOCK = INVALID_SOCKET;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    using socket_t = int
    static constexpr socket_t INVALID_SOCK = -1;
#endif

#include <string>
#include <cstring>

namespace smon {
// Initialize the socket subsystem
inline bool socketInit() {
#ifdef _WIN32
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
#else
    return true;
#endif
}
     
inline void socketCleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}
    
inline void closeSocket(socket_t s) {
    if(s == INVALID_SOCK) return;
#ifdef _WIN32
    closesocket(s);
#else
    ::close(s);
#endif
}
    
// Set a socket to non-blocking mode
inline bool setNonblocking(socket_t s, bool nonblocking) {
#ifdef _WIN32
    u_long mode = nonblocking ? 1 : 0;
    return ioctlsocket(s, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(s, F_GETFL, 0);
    if(flags < 0) return false;
    if(nonblocking) flags |= O_NONBLOCK;
    else            flags &= ~O_NONBLOCK;
    return fcntl(s, F_SETFL, flags) == 0;
#endif
}
}
