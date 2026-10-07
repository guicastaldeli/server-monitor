#pragma once

#include <string>

#define TIMEOUT_MS 500

namespace smon {

    // TCP connect with timeout (ms). Returns true if connected.
    bool tcpCheck(const std::string& host, int port, int timeoutMs = TIMEOUT_MS);

    // "online" or "offline"
    std::string checkStatus(const std::string& host, int port);
    
}