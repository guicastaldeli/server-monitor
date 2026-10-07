#pragma once

#include "tui.hpp"
#include "field.hpp"
#include "platform.hpp"
#include <string>
#include <vector>

#define BROWSER_URL "http://"

namespace smon {

// One endpoint = one service + check.
struct Endpoint {
    std::string service;                // YAML key
    std::string displayUrl;             // Display URL
    std::string checkHost;              // Hostname or IP used for TCP connect
    int checkPort;                      // Port used for TCP connect (0 = not set)
    std::string rawUrl;                 // "host:port" string, for the RAW column
    std::vector<FieldInfo> fields() const { return {
        { "ip", checkHost, [](const std::string& v){ openInBrowser(BROWSER_URL + v); } },
        { "url", displayUrl, [](const std::string& v){ openInBrowser(v); } },
        { "raw", rawUrl, FieldAction{} }
    }; }
};

// A project = a folder containing a yml.
struct Project {
    std::string name;                       // folder name
    std::string path;                       // absolute-ish path to folder
    std::string composePath;                // path to docker-compose.yml.
    std::vector<Endpoint> endpoints;
    std::vector<FieldInfo> fields() const { return {
        { "path", path, [](const std::string&v){ openInBrowser(v); } },
        { "compose", composePath, [](const std::string& v ){ openInFileManager(v); } }
    }; }
};

}