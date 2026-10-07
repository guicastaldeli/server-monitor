#pragma once

#include <string>

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
};

inline const StatusInfo STATUS_TABLE[] = {
    /* Online */ { Status::Online, "Online", "#12df86" },
    /* Offline */ { Status::Offline, "Offline", "#d41e36" },
    /* Unknown */ { Status::Unknown, "UNKNOWN", "#818161"}
};

inline const StatusInfo& infoOf(Status s) {
    for(const auto& si : STATUS_TABLE) {
        if(si.id == s) return si;
    }
    return STATUS_TABLE[2]; // STATUS: Unknown
}

}