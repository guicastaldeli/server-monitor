#pragma once

#include <string>
#include "status.hpp"

#define TIMEOUT_MS 500

namespace smon {

// TCP connect with timeout (ms). Returns true if connected.
bool tcpCheck(const std::string& host, int port, int timeoutMs = TIMEOUT_MS);

// Main check status
Status checkStatus(const std::string& host, int port);
    
}