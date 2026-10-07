#pragma once

#include "endpoint.hpp"
#include "status.hpp"
#include <string>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>

namespace smon {

class StatusMonitor {
public:
    StatusMonitor();
    ~StatusMonitor();

    void registerEndpoints(const std::vector<Project>& projects);
    void start();
    void stop();
    void requestRefresh();

    Status statusOf(const std::string& service,
                            const std::string& rawUrl) const;

private:
    void loop();
    static std::string makeKey(const std::string&,
                                const std::string& rawUrl);
    
    struct Target {
        std::string service;
        std::string rawUrl;
        std::string host;
        int port = 0;
    };

    std::vector<Target> targets;
    mutable std::mutex mutex;
    std::unordered_map<std::string, Status> status;
    std::thread worker;
    std::atomic<bool> stop_{false};
    std::atomic<bool> refresh{false};
};

}