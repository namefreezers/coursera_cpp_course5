#include "transport_catalog.h"

using namespace std;

TransportCatalog::TransportCatalog(vector<Descriptions::InputQuery> data, const Json::Dict &routing_settings_json,
                                   const Json::Dict &render_settings_json, const Json::Dict &yellow_pages_json) {
    auto stops_end = partition(begin(data), end(data), [](const auto &item) {
        return holds_alternative<Descriptions::Stop>(item);
    });

    Descriptions::StopsDict stops_dict;
    for (const auto &item : Range{begin(data), stops_end}) {
        const auto &stop = get<Descriptions::Stop>(item);
        stops_dict[stop.name] = &stop;
        stops_.insert({stop.name, {}});
    }

    Descriptions::BusesDict buses_dict;
    for (const auto &item : Range{stops_end, end(data)}) {
        const auto &bus = get<Descriptions::Bus>(item);

        buses_dict[bus.name] = &bus;
        buses_[bus.name] = Bus{
                bus.stops.size(),
                ComputeUniqueItemsCount(AsRange(bus.stops)),
                ComputeRoadRouteLength(bus.stops, stops_dict),
                ComputeGeoRouteDistance(bus.stops, stops_dict)
        };

        for (const string &stop_name : bus.stops) {
            stops_.at(stop_name).bus_names.insert(bus.name);
        }
    }

    map_renderer_ = MapRenderer(stops_dict, buses_dict, render_settings_json);
    router_ = make_unique<TransportRouter>(stops_dict, buses_dict, routing_settings_json);

    yellow_pages_ = YellowPagesDatabase::YellowPagesDb(yellow_pages_json);
}

TransportCatalog::TransportCatalog(const Serialization::TransportCatalog &serialization_base) {
    for (int i = 0; i < serialization_base.stops_size(); ++i) {
        const Serialization::Stop &cur_serialization_stop = serialization_base.stops(i);

        Stop &cur_stop_ = stops_[cur_serialization_stop.stop_name()];
        for (int bus_name_idx = 0; bus_name_idx < cur_serialization_stop.bus_names_size(); ++bus_name_idx) {
            cur_stop_.bus_names.insert(cur_stop_.bus_names.end(), cur_serialization_stop.bus_names(bus_name_idx));
        }
    }

    for (int i = 0; i < serialization_base.buses_size(); ++i) {
        const Serialization::Bus &cur_serialization_bus = serialization_base.buses(i);

        Bus &cur_bus_ = buses_[cur_serialization_bus.bus_name()];

        cur_bus_.stop_count = cur_serialization_bus.stop_count();
        cur_bus_.unique_stop_count = cur_serialization_bus.unique_stop_count();
        cur_bus_.road_route_length = cur_serialization_bus.road_route_length();
        cur_bus_.geo_route_length = cur_serialization_bus.geo_route_length();
    }

    router_ = make_unique<TransportRouter>(serialization_base.router());

    map_renderer_ = MapRenderer(serialization_base.map_renderer());

    yellow_pages_ = YellowPagesDatabase::YellowPagesDb(serialization_base.yellow_pages());
}

const TransportCatalog::Stop *TransportCatalog::GetStop(const string &name) const {
    return GetValuePointer(stops_, name);
}

const TransportCatalog::Bus *TransportCatalog::GetBus(const string &name) const {
    return GetValuePointer(buses_, name);
}

optional<TransportRouter::RouteInfo> TransportCatalog::FindRoute(const string &stop_from, const string &stop_to) const {
    return router_->FindRoute(stop_from, stop_to);
}

std::string TransportCatalog::RenderMap() const {
    return map_renderer_.RenderMap();
}

std::string TransportCatalog::RenderRoute(const std::vector<TransportRouter::RouteInfo::BusItem> &items) const {
    return map_renderer_.RenderRoute(items);
}
const std::unordered_map<std::string, uint64_t> &TransportCatalog::get_rubric_ids_dict() const {
    return yellow_pages_.get_rubric_ids_dict();
}

std::vector<const YellowPagesDatabase::Company *> TransportCatalog::SearchCompanies(const array<const YellowPagesSearch::CompanyConstraint *, 4> &companies_constraints) const {
    return yellow_pages_.SearchCompanies(companies_constraints);
}

Serialization::TransportCatalog TransportCatalog::SerializeBase() const {
    Serialization::TransportCatalog serialization_base;

    for (const auto&[stop_name, stop] : stops_) {
        Serialization::Stop serialization_stop;
        serialization_stop.set_stop_name(stop_name);
        for (const auto &bus_name: stop.bus_names) {
            serialization_stop.add_bus_names(bus_name);
        }

        *serialization_base.add_stops() = serialization_stop;
    }

    for (const auto&[bus_name, bus] : buses_) {
        Serialization::Bus serialization_bus;
        serialization_bus.set_bus_name(bus_name);
        serialization_bus.set_stop_count(bus.stop_count);
        serialization_bus.set_unique_stop_count(bus.unique_stop_count);
        serialization_bus.set_road_route_length(bus.road_route_length);
        serialization_bus.set_geo_route_length(bus.geo_route_length);

        *serialization_base.add_buses() = serialization_bus;
    }

    *serialization_base.mutable_router() = router_->SerializeRouter();

    *serialization_base.mutable_map_renderer() = map_renderer_.SerializeMapRenderer();

    *serialization_base.mutable_yellow_pages() = yellow_pages_.SerializeYellowPages();

    return serialization_base;
}

int TransportCatalog::ComputeRoadRouteLength(
        const vector<string> &stops,
        const Descriptions::StopsDict &stops_dict
) {
    int result = 0;
    for (size_t i = 1; i < stops.size(); ++i) {
        result += Descriptions::ComputeStopsDistance(*stops_dict.at(stops[i - 1]), *stops_dict.at(stops[i]));
    }
    return result;
}

double TransportCatalog::ComputeGeoRouteDistance(
        const vector<string> &stops,
        const Descriptions::StopsDict &stops_dict
) {
    double result = 0;
    for (size_t i = 1; i < stops.size(); ++i) {
        result += Sphere::Distance(
                stops_dict.at(stops[i - 1])->position, stops_dict.at(stops[i])->position
        );
    }
    return result;
}


