#pragma once

#include <string>
#include <vector>

namespace smon {

enum class Status {
    Online,
    Offline,
    Unknown
};

struct StatusInfo {
    Status id;
    std::string label;
    std::string color;
    std::vector<std::string> styles;
};

inline const StatusInfo STATUS_TABLE[] = {
    /* Online */ { Status::Online, "Online", "#12df86", { "italic", "blink" } },
    /* Offline */ { Status::Offline, "Offline", "#d41e36", { "bold" } },
    /* Unknown */ { Status::Unknown, "UNKNOWN", "#818161", {}}
};

inline const StatusInfo& infoOf(Status s) {
    for(const auto& si : STATUS_TABLE) {
        if(si.id == s) return si;
    }
    return STATUS_TABLE[2]; // STATUS: Unknown
}

}