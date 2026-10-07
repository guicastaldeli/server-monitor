#include "status_monitor.hpp"
#include "status_checker.hpp"
#include <chrono>

namespace smon {

static constexpr int POLL_INTERVAL_MS = 3000;

StatusMonitor::StatusMonitor() = default;
StatusMonitor::~StatusMonitor() {
    stop();
}

std::string StatusMonitor::makeKey(const std::string& service, const std::string& rawUrl) {
    std::string val = service + "|" + rawUrl;
    return val;
}

void StatusMonitor::registerEndpoints(const std::vector<Project>& projects) {
    std::lock_guard<std::mutex> lock(mutex);
    targets.clear();
    status.clear();
    for(const auto& p : projects) {
        for(const auto& e : p.endpoints) {
            Target t;
            t.service = e.service;
            t.rawUrl = e.rawUrl;
            t.host = e.checkHost;
            t.port = e.checkPort;
            targets.push_back(t);
            status[makeKey(e.service, e.rawUrl)] = "unknown";
        }
    }
}

void StatusMonitor::start() {
    stop_.store(false);
    worker = std::thread(&StatusMonitor::loop, this);
}

void StatusMonitor::stop() {
    stop_.store(true);
    if(worker.joinable()) worker.join();
}

void StatusMonitor::requestRefresh() {
    refresh.store(true);
}

std::string StatusMonitor::statusOf(const std::string& service, const std::string& rawUrl) const {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = status.find(makeKey(service, rawUrl));
    if(it == status.end()) return "unknown";
    return it->second;
}

void StatusMonitor::loop() {
    using namespace std::chrono;
    while(!stop_.load()) {
        std::vector<Target> snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex);
            snapshot = targets;
        }

        for(const auto& t : snapshot) {
            if(stop_.load()) return;
            std::string st = checkStatus(t.host, t.port);
            {
                std::lock_guard<std::mutex> lock(mutex);
                status[makeKey(t.service, t.rawUrl)] = st;
            }
        }

        refresh.store(false);

        auto deadline = steady_clock::now() + milliseconds(POLL_INTERVAL_MS);
        while(steady_clock::now() < deadline) {
            if(stop_.load()) return;
            if(refresh.load()) break;
            std::this_thread::sleep_for(milliseconds(100));
        }
    }
}

}