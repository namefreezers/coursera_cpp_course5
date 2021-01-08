#include "transport_router.h"

using namespace std;


TransportRouter::TransportRouter(const Descriptions::StopsDict &stops_dict,
                                 const Descriptions::BusesDict &buses_dict,
                                 const Json::Dict &routing_settings_json)
        : routing_settings_(MakeRoutingSettings(routing_settings_json)) {
    const size_t vertex_count = stops_dict.size() * 2;
    vertices_info_.resize(vertex_count);
    graph_ = BusGraph(vertex_count);

    FillGraphWithStops(stops_dict);
    FillGraphWithBuses(stops_dict, buses_dict);

    router_ = std::make_unique<Router>(graph_);
}

TransportRouter::TransportRouter(const Serialization::TransportRouter &serialization_router) {
//    RoutingSettings routing_settings = 1;
//    BusGraph bus_graph = 2;
//    repeated StopsVertexIds stops_vertex_ids = 3;
//    repeated VertexInfo vertices_info = 4;
//    repeated EdgeType edge_types = 5;
//    repeated BusEdgeInfo bus_edge_infos = 6;
    routing_settings_ = {serialization_router.routing_settings().bus_wait_time(), serialization_router.routing_settings().bus_velocity()};
    graph_ = BusGraph(serialization_router.bus_graph());

    stops_vertex_ids_.reserve(serialization_router.stops_vertex_ids_size());
    for (int stop_vertex_idx = 0; stop_vertex_idx < serialization_router.stops_vertex_ids_size(); ++stop_vertex_idx) {
        const Serialization::StopsVertexIds& serialization_stop_vertex_id = serialization_router.stops_vertex_ids(stop_vertex_idx);

        stops_vertex_ids_[serialization_stop_vertex_id.stop_name()] = {serialization_stop_vertex_id.in(), serialization_stop_vertex_id.out()};
    }

    vertices_info_.reserve(serialization_router.vertices_info_size());
    for (int vertex_info_idx = 0; vertex_info_idx < serialization_router.vertices_info_size(); ++vertex_info_idx) {
        const Serialization::VertexInfo& serialization_vertex_info = serialization_router.vertices_info(vertex_info_idx);

        vertices_info_.push_back({serialization_vertex_info.stop_name()});
    }

    edges_info_.reserve(serialization_router.edge_types_size());
    int bus_edge_idx = 0;
    for (int edge_type_idx = 0; edge_type_idx < serialization_router.edge_types_size(); ++edge_type_idx) {
        const Serialization::EdgeType& serialization_edge_type = serialization_router.edge_types(edge_type_idx);

        switch (serialization_edge_type) {
            case Serialization::EdgeType::BUS:
                edges_info_.emplace_back(BusEdgeInfo{serialization_router.bus_edge_infos(bus_edge_idx).bus_name(), serialization_router.bus_edge_infos(bus_edge_idx).span_count()});
                bus_edge_idx++;
                break;
            case Serialization::EdgeType::WAIT:
                edges_info_.emplace_back(WaitEdgeInfo{});
                break;
        }
    }

    router_ = std::make_unique<Router>(graph_, serialization_router.router());
}

TransportRouter::RoutingSettings TransportRouter::MakeRoutingSettings(const Json::Dict &json) {
    return {
            json.at("bus_wait_time").AsInt(),
            json.at("bus_velocity").AsDouble(),
    };
}

void TransportRouter::FillGraphWithStops(const Descriptions::StopsDict &stops_dict) {
    Graph::VertexId vertex_id = 0;

    for (const auto&[stop_name, _] : stops_dict) {
        auto &vertex_ids = stops_vertex_ids_[stop_name];
        vertex_ids.in = vertex_id++;
        vertex_ids.out = vertex_id++;
        vertices_info_[vertex_ids.in] = {stop_name};
        vertices_info_[vertex_ids.out] = {stop_name};

        edges_info_.emplace_back(WaitEdgeInfo{
        });
        const Graph::EdgeId edge_id = graph_.AddEdge({
                                                             vertex_ids.out,
                                                             vertex_ids.in,
                                                             static_cast<double>(routing_settings_.bus_wait_time)
                                                     });
        assert(edge_id == edges_info_.size() - 1);
    }

    assert(vertex_id == graph_.GetVertexCount());
}

void TransportRouter::FillGraphWithBuses(const Descriptions::StopsDict &stops_dict,
                                         const Descriptions::BusesDict &buses_dict) {
    for (const auto&[_, bus_item] : buses_dict) {
        const auto &bus = *bus_item;
        const size_t stop_count = bus.stops.size();
        if (stop_count <= 1) {
            continue;
        }
        auto compute_distance_from = [&stops_dict, &bus](size_t lhs_idx) {
            return Descriptions::ComputeStopsDistance(*stops_dict.at(bus.stops[lhs_idx]), *stops_dict.at(bus.stops[lhs_idx + 1]));
        };
        for (size_t start_stop_idx = 0; start_stop_idx + 1 < stop_count; ++start_stop_idx) {
            const Graph::VertexId start_vertex = stops_vertex_ids_[bus.stops[start_stop_idx]].in;
            int total_distance = 0;
            for (size_t finish_stop_idx = start_stop_idx + 1; finish_stop_idx < stop_count; ++finish_stop_idx) {
                total_distance += compute_distance_from(finish_stop_idx - 1);
                edges_info_.push_back(BusEdgeInfo{
                        .bus_name = bus.name,
                        .span_count = finish_stop_idx - start_stop_idx,

                        // Render Route
                        .start_stop_idx = start_stop_idx,
                        .finish_stop_idx = finish_stop_idx,
                });
                const Graph::EdgeId edge_id = graph_.AddEdge({
                                                                     start_vertex,
                                                                     stops_vertex_ids_[bus.stops[finish_stop_idx]].out,
                                                                     total_distance * 1.0 / (routing_settings_.bus_velocity * 1000.0 / 60)  // m / (km/h * 1000 / 60) = min
                                                             });
                assert(edge_id == edges_info_.size() - 1);
            }
        }
    }
}

optional<TransportRouter::RouteInfo> TransportRouter::FindRoute(const string &stop_from, const string &stop_to) const {
    const Graph::VertexId vertex_from = stops_vertex_ids_.at(stop_from).out;
    const Graph::VertexId vertex_to = stops_vertex_ids_.at(stop_to).out;
    const auto route = router_->BuildRoute(vertex_from, vertex_to);
    if (!route) {
        return nullopt;
    }

    RouteInfo route_info = {.total_time = route->weight};
    route_info.items.reserve(route->edge_count);
    for (size_t edge_idx = 0; edge_idx < route->edge_count; ++edge_idx) {
        const Graph::EdgeId edge_id = router_->GetRouteEdge(route->id, edge_idx);
        const auto &edge = graph_.GetEdge(edge_id);
        const auto &edge_info = edges_info_[edge_id];
        if (holds_alternative<BusEdgeInfo>(edge_info)) {
            const BusEdgeInfo &bus_edge_info = get<BusEdgeInfo>(edge_info);
            route_info.items.emplace_back(RouteInfo::BusItem{
                    .bus_name = bus_edge_info.bus_name,
                    .time = edge.weight,
                    .span_count = bus_edge_info.span_count,

                    // Render Route
                    .start_stop_idx = bus_edge_info.start_stop_idx,
                    .finish_stop_idx = bus_edge_info.finish_stop_idx,
            });
        } else {
            const Graph::VertexId vertex_id = edge.from;
            route_info.items.emplace_back(RouteInfo::WaitItem{
                    .stop_name = vertices_info_[vertex_id].stop_name,
                    .time = edge.weight,
            });
        }
    }

    // Releasing in destructor of some proxy object would be better,
    // but we do not expect exceptions in normal workflow
    router_->ReleaseRoute(route->id);
    return route_info;
}

Serialization::TransportRouter TransportRouter::SerializeRouter() const {
    Serialization::TransportRouter serialization_router;

    *serialization_router.mutable_routing_settings() = routing_settings_.SerializeRoutingSettings();

    *serialization_router.mutable_bus_graph() = graph_.SerializeBusGraph();

    for (const auto&[stop_name, cur_stop_vertex_ids] : stops_vertex_ids_) {
        Serialization::StopsVertexIds serialization_stop_vertex_id;
        serialization_stop_vertex_id.set_stop_name(stop_name);
        serialization_stop_vertex_id.set_in(cur_stop_vertex_ids.in);
        serialization_stop_vertex_id.set_out(cur_stop_vertex_ids.out);

        *serialization_router.add_stops_vertex_ids() = serialization_stop_vertex_id;
    }

    for (const auto &vertex_info : vertices_info_) {
        Serialization::VertexInfo serialization_vertex_info;
        serialization_vertex_info.set_stop_name(vertex_info.stop_name);
        *serialization_router.add_vertices_info() = serialization_vertex_info;
    }

    for (const EdgeInfo &edge_info : edges_info_) {
        if (holds_alternative<BusEdgeInfo>(edge_info)) {
            serialization_router.add_edge_types(Serialization::EdgeType::BUS);

            Serialization::BusEdgeInfo serialization_bus_edge_info;
            serialization_bus_edge_info.set_bus_name(get<BusEdgeInfo>(edge_info).bus_name);
            serialization_bus_edge_info.set_span_count(get<BusEdgeInfo>(edge_info).span_count);
            *serialization_router.add_bus_edge_infos() = serialization_bus_edge_info;
        } else {
            serialization_router.add_edge_types(Serialization::EdgeType::WAIT);
        }
    }

    *serialization_router.mutable_router() = router_->SerializeRouter();

    return serialization_router;
}