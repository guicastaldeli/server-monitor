#pragma once
#include <string>
#include <vector>

namespace smon {

// One endpoint = one service + check.
struct Endpoint {
    std::string service;                // YAML key
    std::string displayUrl;             // Display URL
    std::string checkHost;              // Hostname or IP used for TCP connect
    int checkPort;                      // Port used for TCP connect (0 = not set)
    std::string rawUrl;                 // "host:port" string, for the RAW column
};

// A project = a folder containing a yml.
struct Project {
    std::string name;                       // folder name
    std::string path;                       // absolute-ish path to folder
    std::string composePath;                // path to docker-compose.yml.
    std::vector<Endpoint> endpoints;
};

}