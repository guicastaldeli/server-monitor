#pragma once

#include "endpoint.hpp"
#include <string>
#include <vector>

namespace smon {

// Parse a docker-compose.yml and
// return the endpoints it decalres.
std::vector<Endpoint> parseCompose(const std::string& path);

}