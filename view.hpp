#pragma once

#include "endpoint.hpp"
#include "field.hpp"
#include "status.hpp"
#include <string>
#include <vector>

namespace smon {
    
    struct Row {
        bool isEndpoint = false;
        std::vector<FieldInfo> fields;
        Status status = Status::Unknown;
        int sourceIndex = 0;
    };

    struct ProjectView {
        std::string name;
        std::vector<Row> rows;
    };

    std::vector<ProjectView> buildViews(const std::vector<Project>& projects);
    
}