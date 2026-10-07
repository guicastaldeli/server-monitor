/******************
 * 
 * 
 * Cross platform socket initialization + connect helpers
 * 
 * 
 *************/
#pragma once

#include <string>
#include <vector>

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
    using socket_t = int;
    static constexpr socket_t INVALID_SOCK = -1;
#endif

namespace smon {

/**
 * 
 * Sockets
 * 
 */
bool socketInit();
void socketCleanup();
void closeSocket(socket_t s);
bool setNonblocking(socket_t s, bool nonblocking);

/**
 * 
 * External openers
 * 
 */
bool openInBrowser(const std::string& url);
bool openInFileManager(const std::string& path);

}
