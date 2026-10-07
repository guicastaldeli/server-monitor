#include "view.hpp"

namespace smon {

    std::vector<ProjectView> buildViews(const std::vector<Project>& projects) {
        std::vector<ProjectView> out;
        out.reserve(projects.size());

        for(const auto& p : projects) {
            ProjectView pv;
            pv.name = p.name;

            {
                Row r;
                r.isEndpoint = false;
                r.fields = p.fields();
                pv.rows.push_back(std::move(r));
            }

            for(size_t i = 0; i < p.endpoints.size(); i++) {
                Row r;
                r.isEndpoint = true;
                r.fields = p.endpoints[i].fields();
                r.sourceIndex = (int)i;
                pv.rows.push_back(std::move(r));
            }

            out.push_back(std::move(pv));
        }

        return out;
    }
    
}