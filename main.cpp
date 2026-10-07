/**************************
 * 
 * 
 * 
 * Main project entry point
 * with main() method. 
 * 
 * 
 * 
 ***************************/

#include "platform.hpp"
#include "compose_parser.hpp"
#include "project_scanner.hpp"
#include "status_monitor.hpp"
#include "view.hpp"
#include "tui.hpp"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cerr << "usage: server_monitor <root_folder>\n";
        return 1;
    }

    std::string root = argv[1];
    
    if(!smon::socketInit()) {
        std::cerr << "err: failed to initalize sockets\n";
        return 1;
    }

    std::vector<smon::Project> projects = smon::scanProjects(root);
    for(auto& p : projects) {
        p.endpoints = smon::parseCompose(p.composePath);
    }

    smon::StatusMonitor statusMonitor;
    statusMonitor.registerEndpoints(projects);
    statusMonitor.start();

    std::vector<smon::ProjectView> views = smon::buildViews(projects);
    
    smon::render(root, projects, views, statusMonitor);

    statusMonitor.stop();
    smon::socketCleanup();
    
    return 0;
}