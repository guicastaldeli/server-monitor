#include "compose_parser.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace smon {
namespace detail {

static std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}

static int indentOf(const std::string& line) {
    int i = 0;
    while(i < (int)line.size() && line[i] == ' ') ++i;
    return i;
}

static std::string stripQuotes(const std::string& s) {
    if(s.size() >= 2 &&
        ((s.front() == '"' && s.back() == '"') ||
            (s.front() == '\'' && s.back() == '\''))) {
                return s.substr(1, s.size() - 2);
            }

    return s;
}

/**
 * 
 * Port
 * 
 */
struct PortSpec {
    int hostPort = -1;
    std::string bindIp;
};

static PortSpec parsePort(const std::string& entryIn) {
    PortSpec out;
    std::string entry = stripQuotes(trim(entryIn));
    
    auto slash = entry.find('/');
    if(slash != std::string::npos) entry = entry.substr(0, slash);

    std::vector<std::string> parts;
    std::stringstream ss(entry);
    std::string tok;
    while(std::getline(ss, tok, ':')) parts.push_back(tok);

    if(parts.empty()) return out;
    for(auto& p : parts) if (p.find('-') != std::string::npos) return out;

    if(parts.size() == 1) {
        try { out.hostPort = std::stoi(parts[0]); }
        catch(...) { out.hostPort = -1; }
    } else if(parts.size() == 2) {
        try { out.hostPort = std::stoi(parts[0]); } 
        catch(...) { out.hostPort = -1; }
    } else if(parts.size() >= 3) {
        out.bindIp = parts[0];
        try { out.hostPort = std::stoi(parts[1]); } 
        catch(...) { out.hostPort = -1;}
    }

    return out;
}

/**
 * 
 * Url
 * 
 */
struct UrlSpec {
    std::string host;
    int port = 0;
    bool hasPort = false;
};

static UrlSpec parseUrl(const std::string& raw) {
    UrlSpec out;
    std::string s = stripQuotes(trim(raw));
    
    auto scheme = s.find("://");
    if(scheme != std::string::npos) s = s.substr(scheme + 3);
    auto slash = s.find('/');
    if(slash != std::string::npos) s = s.substr(0, slash);
    auto colon = s.rfind(':');
    if(colon != std::string::npos) {
        out.host = s.substr(0, colon);
        try {
            out.port = std::stoi(s.substr(colon + 1));
            out.hasPort = true;
        } catch(...) { 
            out.port = 0; 
            out.hasPort = false; 
        }
    } else {
        out.host = s;
    }

    return out;
}
} // namespace detail...

std::vector<Endpoint> parseCompose(const std::string& path) {
    std::vector<Endpoint> result;

    std::ifstream in(path);
    if(!in) return result;

    std::vector<std::string> lines;
    std::string line;
    while(std::getline(in, line)) {
        if(!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }

    size_t servicesStart = std::string::npos;
    for(size_t i = 0; i < lines.size(); ++i) {
        if(detail::indentOf(lines[i]) == 0 && detail::trim(lines[i]) == "services:") {
            servicesStart = i + 1;
            break;
        }
    }
    if(servicesStart == std::string::npos) return result;

    /**
     * 
     * Service
     * 
     */
    struct ServiceBlock {
        std::string name;
        size_t start;
        size_t end;
    };

    std::vector<ServiceBlock> blocks;
    size_t i = servicesStart;
    while(i < lines.size()) {
        const std::string& ln = lines[i];
        if(detail::trim(ln).empty()) { ++i; continue; }
        
        int ind = detail::indentOf(ln);
        if(ind == 0) break;
        if(ind == 2) {
            std::string t = detail::trim(ln);
            if(!t.empty() && t.back() == ':') {
                ServiceBlock b;
                b.name = t.substr(0, t.size() - 1);
                b.start = i + 1;
                size_t j = i + 1;
                while(j < lines.size()) {
                    if(detail::trim(lines[j]).empty()) { ++j; continue; }
                    int jind = detail::indentOf(lines[j]);
                    if(jind == 0) break;
                    if(jind == 2 && lines[j].back() == ':') break;
                    ++j;
                }
                b.end = j;
                blocks.push_back(b);
                i = j;
                continue;
            }
        }
        ++i;
    }

    for(auto& b : blocks) {
        Endpoint ep;
        ep.service = b.name;
        std::string urlValue;
        std::vector<detail::PortSpec> portSpecs;

        bool inPorts = false;
        int portsIdent = -1;

        for(size_t k = b.start; k < b.end; ++k) {
            const std::string& ln = lines[k];
            std::string t = detail::trim(ln);
            if(t.empty() || t[0] == '#') continue;
            int ind = detail::indentOf(ln);
            if(inPorts) {
                if(ind > portsIdent && !t.empty() && t[0] == '-') {
                    std::string entry = detail::trim(t.substr(1));
                    auto spec = detail::parsePort(entry);
                    if(spec.hostPort > 0) portSpecs.push_back(spec);
                    continue;
                } else {
                    inPorts = false;
                }
            }

            if(t.rfind("url:", 0) == 0) {
                urlValue = detail::trim(t.substr(4));
                continue;
            }
            if(t == "ports:") {
                inPorts = true;
                portsIdent = ind;
                continue;
            }
        }

        if(!urlValue.empty()) {
            auto u = detail::parseUrl(urlValue);
            ep.displayUrl = detail::stripQuotes(urlValue);

            if(u.hasPort) {
                ep.checkHost = u.host;
                ep.checkPort = u.port;
            } else {
                ep.checkHost = u.host;
                std::string raw = detail::stripQuotes(urlValue);
                bool https = raw.find("https://", 0) == 0;
                ep.checkPort = https ? 443 : 80;
            }
            if(!u.hasPort && !portSpecs.empty()) {
                ep.checkPort = portSpecs.front().hostPort;
            }

            ep.rawUrl = ep.checkHost + ":" + std::to_string(ep.checkPort);
            result.push_back(ep);
        } else if(!portSpecs.empty()) {
            const auto& ps = portSpecs.front();
            std::string host = ps.bindIp.empty() ? "localhost" : ps.bindIp;
            ep.checkHost = host;
            ep.checkPort = ps.hostPort;
            ep.displayUrl = "http://" + host + ":" + std::to_string(ps.hostPort);
            ep.rawUrl = host + ":" + std::to_string(ps.hostPort);
            result.push_back(ep);
        }
    }

    return result;
}

}