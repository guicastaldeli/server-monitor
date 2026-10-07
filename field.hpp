#pragma once

#include <string>
#include <functional>

namespace smon {

using FieldAction = std::function<void(const std::string&)>;

struct FieldInfo {
    std::string label;
    std::string value;
    FieldAction action;
    bool isUrl = false;
};

}