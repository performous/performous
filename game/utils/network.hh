#pragma once

#include <string>
#include <vector>

enum class IPFilter { IPV4, IPV6, Both};

std::vector<std::string> getLocalIPAddresses(bool noLocalHost = true, IPFilter = IPFilter::Both);
std::vector<std::string> getLocalIPAddresses(IPFilter, bool noLocalHost = true);
