#include "project_scanner.hpp"
#include <filesystem>

namespace fs = std::filesystem;
namespace smon {

bool isComposeFile(const std::string& filename) {
    if(filename.rfind("docker-compose.", 0) != 0) return false;
    fs::path p(filename);
    std::string ext = p.extension().string();
    return ext == ".yml" || ext == ".yaml";
}

std::vector<Project> scanProjects(const std::string& root) {
    std::vector<Project> projects;
    std::error_code err;

    fs::path rootPath(root);
    if(!fs::exists(rootPath, err) || err) return projects;

    fs::recursive_directory_iterator it(
        rootPath,
        fs::directory_options::skip_permission_denied,
        err);
    if(err) return projects;

    fs::recursive_directory_iterator end;
    for(; it != end; it.increment(err)) {
        if(err) { err.clear(); continue; }

        const fs::directory_entry& entry = *it;
        if(!entry.is_regular_file(err) || err) { err.clear(); continue; }
        
        std::string fname = entry.path().filename().string();
        if(isComposeFile(fname)) continue;

        Project p;
        p.path = entry.path().parent_path().string();
        p.composePath = entry.path().string();
        p.name = entry.path().parent_path().filename().string();
        projects.push_back(std::move(p));
    }

    return projects;
}

}