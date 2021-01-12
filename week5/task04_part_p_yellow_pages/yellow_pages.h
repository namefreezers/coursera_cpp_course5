#pragma once

#include "json.h"

#include <unordered_map>

class Company

class YellowPages {
public:
    YellowPages() = default;
    explicit YellowPages(const Json::Dict &yellow_pages_json);

private:
    std::unordered_map<std::string, uint64_t> rubrics_;
};