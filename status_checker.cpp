#include "status_checker.hpp"
#include "platform.hpp"
#include <string>

namespace smon {

    bool tcpCheck(const std::string& host, int port, int timeoutMs) {
        if(port <= 0) return false;

        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        addrinfo* res = nullptr;

        std::string port_str = std::to_string(port);
        if(getaddrinfo(
            host.c_str(),
            port_str.c_str(),
            &hints,
            &res
        ) != 0) {
            return false;
        }

        bool ok = false;
        for(addrinfo* ai = res; ai != nullptr && !ok; ai = ai->ai_next) {
            socket_t s = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
            if(s == INVALID_SOCK) continue;

            setNonblocking(s, true);

            int rc = ::connect(s,
            #ifdef _WIN32
                ai->ai_addr, (int)ai->ai_addrlen
            #else
                ai->ai_addr, ai->ai_addrlen
            #endif
            );
            if(rc == 0) {
                ok = true;
            } else {
                fd_set wfds;
                FD_ZERO(&wfds);
                FD_SET(s, &wfds);
                timeval tv;
                tv.tv_sec = timeoutMs / 1000;
                tv.tv_usec = (timeoutMs % 1000) * 1000;

            #ifdef _WIN32
                int sel = ::select(0, nullptr, &wfds, nullptr, &tv);
            #else
                int sel = ::select(s+1, nullptr, &wfds, nullptr, &tv);
            #endif
                
                if(sel > 0 && FD_ISSET(s, &wfds)) {
                    int err = 0;

            #ifdef _WIN32
                    int len = sizeof(err);
                    getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
            #else
                    socklen_t len = sizeof(err);
                    getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &len);
            #endif
                    
                    if(err == 0) ok = true;
                }
            }

            closeSocket(s);
        }

        freeaddrinfo(res);
        return ok;
    }

    std::string checkStatus(const std::string& host, int port) {
        std::string on = "online";
        std::string off = "offline";
        std::string val = tcpCheck(host, port, TIMEOUT_MS) ? on : off;
        return val;
    }
}