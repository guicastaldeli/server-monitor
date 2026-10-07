#pragma once

#include <string>

namespace smon {

struct RGB {
    int r = 0;
    int g = 0;
    int b = 0;
};

inline const char* ANSI_RESET = "\x1b[0m";

/**
 * 
 * Parse "#RRGGBB" or "RRGGBB" or "0xRRGGBB".
 * Returns {0, 0, 0} on failure.
 * 
 */
inline RGB parseHex(const std::string& hexIn) {
    RGB out;
    std::string hex = hexIn;

    if(!hex.empty() && hex[0] == '#') hex = hex.substr(1);
    if(hex.size() > 2 && hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')) hex = hex.substr(2);
    if(hex.size() != 6) return out;

    auto hexVal = [](char c) -> int {
        if(c >= '0' && c <= '9') return c - '0';
        if(c >= 'a' && c <= 'f') return c - 'a' + 10;
        if(c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };

    int vals[6];
    for(size_t i = 0; i < 6; i++) {
        vals[i] = hexVal(hex[i]);
        if(vals[i] < 0) return out;
    }

    out.r = (vals[0] << 4) | vals[1];
    out.g = (vals[2] << 4) | vals[3];
    out.b = (vals[4] << 4) | vals[5];
    return out;
}

/**
 * 
 * To ANSI
 * 
 */
inline std::string toAnsiFg(const RGB& c) {
    return "\x1b[38;2;" + std::to_string(c.r) + ";"
                    + std::to_string(c.g) + ";"
                    + std::to_string(c.b) + "m";
}

inline std::string toAnsiBg(const RGB& c) {
    return "\x1b[48;2;" + std::to_string(c.r) + ";"
                    + std::to_string(c.g) + ";"
                    + std::to_string(c.b) + "m";
}

/**
 * 
 * HEX to ANSI
 * 
 */
inline std::string hexToAnsiFg(const std::string& hex) {
    std::string val = toAnsiFg(parseHex(hex));
    return val;
}

}