#pragma once

#include <array>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "transport_catalog.pb.h"

#include "descriptions.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "yellow_pages.h"

namespace Responses {
    struct Stop {
        std::set<std::string> bus_names;
    };

    struct Bus {
        size_t stop_count = 0;
        size_t unique_stop_count = 0;
        int road_route_length = 0;
        double geo_route_length = 0.0;
    };
}

class TransportCatalog {
private:
    using Bus = Responses::Bus;
    using Stop = Responses::Stop;

public:
    TransportCatalog(std::vector<Descriptions::InputQuery> data, const Json::Dict &routing_settings_json, const Json::Dict &render_settings_json, const Json::Dict &yellow_pages_json);

    TransportCatalog(const Serialization::TransportCatalog &serialization_base);

    const Stop *GetStop(const std::string &name) const;

    const Bus *GetBus(const std::string &name) const;

    std::optional<TransportRouter::RouteInfo> FindRoute(const std::string &stop_from, const std::string &stop_to) const;

    std::string RenderMap() const;

    std::string RenderRoute(const std::vector<TransportRouter::RouteInfo::BusItem> &items) const;

    const std::unordered_map<std::string, uint64_t> &get_rubric_ids_dict() const;

    std::vector<const YellowPagesDatabase::Company *> SearchCompanies(const std::array<const YellowPagesSearch::CompanyConstraint *, 4> &companies_constraints) const;

    Serialization::TransportCatalog SerializeBase() const;

private:

    static int ComputeRoadRouteLength(
            const std::vector<std::string> &stops,
            const Descriptions::StopsDict &stops_dict
    );

    static double ComputeGeoRouteDistance(
            const std::vector<std::string> &stops,
            const Descriptions::StopsDict &stops_dict
    );

    std::unordered_map<std::string, Stop> stops_;
    std::unordered_map<std::string, Bus> buses_;

    MapRenderer map_renderer_;
    std::unique_ptr<TransportRouter> router_;

    YellowPagesDatabase::YellowPagesDb yellow_pages_;
};
