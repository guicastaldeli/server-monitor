#pragma once

#include "endpoint.hpp"
#include "field.hpp"
#include <string>
#include <vector>

namespace smon {
    
    struct Row {
        bool isEndpoint = false;
        std::vector<FieldInfo> fields;
        std::string status;
        int sourceIndex = 0;
    };

    struct ProjectView {
        std::string name;
        std::vector<Row> rows;
    };

    std::vector<ProjectView> buildViews(const std::vector<Project>& projects);
    
}