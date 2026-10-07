#pragma once

#include <string>
#include <vector>
#include "endpoint.hpp"

namespace smon {

    bool isComposeFile(const std::string& filename);
    //Recursively scan root. One project per folder
    // containing a compose file.
    std::vector<Project> scanProjects(const std::string& root);

}