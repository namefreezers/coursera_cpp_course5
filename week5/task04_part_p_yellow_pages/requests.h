#pragma once

#include <string>

#include "transport_catalog.h"
#include "json.h"
#include "yellow_pages_search.h"

namespace Requests {
    struct Stop {
        std::string name;

        Json::Dict Process(const TransportCatalog &db) const;
    };

    struct Bus {
        std::string name;

        Json::Dict Process(const TransportCatalog &db) const;
    };

    struct Route {
        std::string stop_from;
        std::string stop_to;

        Json::Dict Process(const TransportCatalog &db) const;
    };

    struct FindCompanies {
        FindCompanies(const std::vector<Json::Node> &names_json,
                      const std::vector<Json::Node> &phones_json,
                      const std::vector<Json::Node> &urls_json,
                      const std::vector<Json::Node> &rubrics_json,
                      const std::unordered_map<std::string, uint64_t> &rubric_ids_dict) : names_constraint_(names_json),
                                                                                          phones_constraint_(phones_json),
                                                                                          urls_constraint_(urls_json),
                                                                                          rubrics_constraint_(rubrics_json, rubric_ids_dict) {}

        Json::Dict Process(const TransportCatalog &db) const;

    private:
        YellowPagesSearch::CompanyNameConstraint names_constraint_;
        YellowPagesSearch::CompanyPhoneConstraint phones_constraint_;
        YellowPagesSearch::CompanyUrlConstraint urls_constraint_;
        YellowPagesSearch::CompanyRubricConstraint rubrics_constraint_;
    };

    struct Map {
        Json::Dict Process(const TransportCatalog &db) const;
    };

    std::variant<Stop, Bus, Route, FindCompanies, Map> Read(const Json::Dict &attrs);

    std::vector<Json::Node> ProcessAll(const TransportCatalog &db, const std::vector<Json::Node> &requests);
}
