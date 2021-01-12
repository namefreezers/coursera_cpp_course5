#include "map_renderer.h"

#include <algorithm>
#include <functional>
#include <unordered_set>

using namespace std;

RenderSettings::RenderSettings() {}

RenderSettings::RenderSettings(const Json::Dict &render_settings_json) {
    width = render_settings_json.at("width").AsDouble();
    height = render_settings_json.at("height").AsDouble();
    padding = render_settings_json.at("padding").AsDouble();
    stop_radius = render_settings_json.at("stop_radius").AsDouble();
    line_width = render_settings_json.at("line_width").AsDouble();
    stop_label_font_size = render_settings_json.at("stop_label_font_size").AsInt();
    stop_label_offset = {render_settings_json.at("stop_label_offset").AsArray()[0].AsDouble(), render_settings_json.at("stop_label_offset").AsArray()[1].AsDouble()};
    underlayer_color = Svg::Color(render_settings_json.at("underlayer_color"));
    underlayer_width = render_settings_json.at("underlayer_width").AsDouble();
    for (const auto &c : render_settings_json.at("color_palette").AsArray()) {
        color_palette.emplace_back(c);
    }
    bus_label_font_size = render_settings_json.at("bus_label_font_size").AsInt();
    bus_label_offset = {render_settings_json.at("bus_label_offset").AsArray()[0].AsDouble(), render_settings_json.at("bus_label_offset").AsArray()[1].AsDouble()};
    for (const auto &l_node : render_settings_json.at("layers").AsArray()) {
        layers.push_back(l_node.AsString());
    }
    outer_margin = render_settings_json.at("outer_margin").AsDouble();
}


// ===========================================================================================================================================
// ========================================================= PointConverter ==================================================================
// ===========================================================================================================================================

PointConverter::PointConverter() {}

PointConverter::PointConverter(const std::map<std::string, Sphere::Point> &stop_coords, const RenderSettings &renderSettings) {
    const auto&[min_lat_pair, max_lat_pair] = minmax_element(begin(stop_coords), end(stop_coords),
                                                             [](const pair<std::string, Sphere::Point> &lhs, const pair<std::string, Sphere::Point> &rhs) {
                                                                 return lhs.second.latitude < rhs.second.latitude;
                                                             });
    const auto&[min_lon_pair, max_lon_pair] = minmax_element(begin(stop_coords), end(stop_coords),
                                                             [](const pair<std::string, Sphere::Point> &lhs, const pair<std::string, Sphere::Point> &rhs) {
                                                                 return lhs.second.longitude < rhs.second.longitude;
                                                             });

    optional<double> width_zoom_coef = (max_lon_pair->second.longitude - min_lon_pair->second.longitude) == 0 ? nullopt : make_optional(
            (renderSettings.width - 2 * renderSettings.padding) / (max_lon_pair->second.longitude - min_lon_pair->second.longitude));
    optional<double> height_zoom_coef = (max_lat_pair->second.latitude - min_lat_pair->second.latitude) == 0 ? nullopt : make_optional(
            (renderSettings.height - 2 * renderSettings.padding) / (max_lat_pair->second.latitude - min_lat_pair->second.latitude));

    if (!width_zoom_coef && !height_zoom_coef) {
        zoom_coef = 0;
    } else if (!width_zoom_coef) {
        zoom_coef = *height_zoom_coef;
    } else if (!height_zoom_coef) {
        zoom_coef = *width_zoom_coef;
    } else {
        zoom_coef = min(*width_zoom_coef, *height_zoom_coef);
    }
    padding = renderSettings.padding;
    min_lon = min_lon_pair->second.longitude;
    max_lat = max_lat_pair->second.latitude;
}

Svg::Point PointConverter::operator()(Sphere::Point to_convert) const {
    return {(to_convert.longitude - min_lon) * zoom_coef + padding, (max_lat - to_convert.latitude) * zoom_coef + padding};
}


// ===========================================================================================================================================
// ========================================================= PointConverterFlattened =========================================================
// ===========================================================================================================================================

PointConverterFlattened::PointConverterFlattened() {}

PointConverterFlattened::PointConverterFlattened(const std::map<std::string, Sphere::Point> &stop_coords, const RenderSettings &renderSettings) {
    padding = renderSettings.padding;
    height = renderSettings.height;

    if (stop_coords.size() < 2) {
        x_step = 0, y_step = 0;
    } else {
        x_step = (renderSettings.width - 2 * renderSettings.padding) / static_cast<double>(stop_coords.size() - 1);
        y_step = (renderSettings.height - 2 * renderSettings.padding) / static_cast<double>(stop_coords.size() - 1);
    }

    x_coords_sorted.reserve(stop_coords.size());
    transform(stop_coords.begin(), stop_coords.end(), back_inserter(x_coords_sorted), [](const pair<std::string, Sphere::Point> &m_entry) { return m_entry.second.longitude; });
    sort(x_coords_sorted.begin(), x_coords_sorted.end());
    y_coords_sorted.reserve(stop_coords.size());
    transform(stop_coords.begin(), stop_coords.end(), back_inserter(y_coords_sorted), [](const pair<std::string, Sphere::Point> &m_entry) { return m_entry.second.latitude; });
    sort(y_coords_sorted.begin(), y_coords_sorted.end());
}

Svg::Point PointConverterFlattened::operator()(Sphere::Point to_convert) const {
    int idx_x = lower_bound(x_coords_sorted.begin(), x_coords_sorted.end(), to_convert.longitude) - x_coords_sorted.begin();
    int idx_y = lower_bound(y_coords_sorted.begin(), y_coords_sorted.end(), to_convert.latitude) - y_coords_sorted.begin();
    return {idx_x * x_step + padding, height - padding - idx_y * y_step};
}


// ===========================================================================================================================================
// ========================================================= PointConverterFlattenCompress =======================================================
// ===========================================================================================================================================

PointConverterFlattenCompress::PointConverterFlattenCompress() {}

bool is_next_stop_adjacent(const vector<string> &cur_coord_stops, const string &next_stop, const unordered_map<string, unordered_set<string>> &adjacent_stops) {
    auto it_next_stop_adjacent = adjacent_stops.find(next_stop);
    if (it_next_stop_adjacent == adjacent_stops.end()) {  // if stop not in bus -> not adjacent with any
        return false;
    }
    const unordered_set<string> &next_stop_adjacent = it_next_stop_adjacent->second;

    // if adjacent with any of
    return any_of(
            cur_coord_stops.begin(),
            cur_coord_stops.end(),
            [&next_stop_adjacent](const string &cur_coord_stop) {
                return next_stop_adjacent.count(cur_coord_stop);
            }
    );
}

PointConverterFlattenCompress::PointConverterFlattenCompress(const std::map<std::string, Sphere::Point> &stop_coords, const Descriptions::BusesDict &buses_dict, const RenderSettings &renderSettings) {
    unordered_map<string, unordered_set<string>> adjacent_stops;
    for (const auto&[bus_name, desc_bus] : buses_dict) {
        for (auto[it_prev, it_next] = make_tuple(desc_bus->stops.begin(), next(desc_bus->stops.begin())); it_next != desc_bus->stops.end(); it_prev++, it_next++) {
            adjacent_stops[*it_prev].insert(*it_next);
            adjacent_stops[*it_next].insert(*it_prev);
        }
    }

    padding = renderSettings.padding;
    height = renderSettings.height;

    std::vector<pair<std::string, Sphere::Point>> x_stops_sorted(stop_coords.begin(), stop_coords.end()), y_stops_sorted(stop_coords.begin(), stop_coords.end());
    sort(x_stops_sorted.begin(), x_stops_sorted.end(),
         [](const pair<std::string, Sphere::Point> &lhs, const pair<std::string, Sphere::Point> &rhs) { return lhs.second.longitude < rhs.second.longitude; });
    sort(y_stops_sorted.begin(), y_stops_sorted.end(),
         [](const pair<std::string, Sphere::Point> &lhs, const pair<std::string, Sphere::Point> &rhs) { return lhs.second.latitude < rhs.second.latitude; });

    vector<string> cur_coord_stops = {x_stops_sorted.front().first};
    x_coords_sorted.push_back(x_stops_sorted.front().second.longitude);
    for (auto it_prev = x_stops_sorted.begin(), it_next = next(x_stops_sorted.begin()); it_next != x_stops_sorted.end(); it_prev++, it_next++) {
        const string &s1 = it_prev->first, s2 = it_next->first;
        if (is_next_stop_adjacent(cur_coord_stops, it_next->first, adjacent_stops)) {  // if adjacent with any of
            x_coords_sorted.push_back(it_next->second.longitude);
            cur_coord_stops.clear();
        }
        cur_coord_stops.push_back(it_next->first);
    }

    cur_coord_stops = {y_stops_sorted.front().first};
    y_coords_sorted.push_back(y_stops_sorted.front().second.latitude);
    for (auto it_prev = y_stops_sorted.begin(), it_next = next(y_stops_sorted.begin()); it_next != y_stops_sorted.end(); it_prev++, it_next++) {
        if (is_next_stop_adjacent(cur_coord_stops, it_next->first, adjacent_stops)) {  // if adjacent with any of
            y_coords_sorted.push_back(it_next->second.latitude);
            cur_coord_stops.clear();
        }
        cur_coord_stops.push_back(it_next->first);
    }

    x_step = x_coords_sorted.size() < 2 ? 0 : (renderSettings.width - 2 * renderSettings.padding) / static_cast<double>(x_coords_sorted.size() - 1);
    y_step = y_coords_sorted.size() < 2 ? 0 : (renderSettings.height - 2 * renderSettings.padding) / static_cast<double>(y_coords_sorted.size() - 1);
}

Svg::Point PointConverterFlattenCompress::operator()(Sphere::Point to_convert) const {
    int idx_x = prev(upper_bound(x_coords_sorted.begin(), x_coords_sorted.end(), to_convert.longitude)) - x_coords_sorted.begin();
    int idx_y = prev(upper_bound(y_coords_sorted.begin(), y_coords_sorted.end(), to_convert.latitude)) - y_coords_sorted.begin();
    return {idx_x * x_step + padding, height - padding - idx_y * y_step};
}


// ===========================================================================================================================================
// ========================================================= PointConverterFlattenCompressRoutes =======================================================
// ===========================================================================================================================================

PointConverterFlattenCompressRoutes::PointConverterFlattenCompressRoutes() {}

PointConverterFlattenCompressRoutes::PointConverterFlattenCompressRoutes(const std::map<std::string, Sphere::Point> &stop_coords, const Descriptions::BusesDict &buses_dict,
                                                                         const RenderSettings &renderSettings) {
    // adjacent stops
    unordered_map<string, unordered_set<string>> adjacent_stops;
    for (const auto&[bus_name, desc_bus] : buses_dict) {
        for (auto[it_prev, it_next] = make_tuple(desc_bus->stops.begin(), next(desc_bus->stops.begin())); it_next != desc_bus->stops.end(); it_prev++, it_next++) {
            adjacent_stops[*it_prev].insert(*it_next);
            adjacent_stops[*it_next].insert(*it_prev);
        }
    }

    padding = renderSettings.padding;
    height = renderSettings.height;

    std::vector<pair<std::string, Sphere::Point>> x_stops_sorted(stop_coords.begin(), stop_coords.end()), y_stops_sorted(stop_coords.begin(), stop_coords.end());
    sort(x_stops_sorted.begin(), x_stops_sorted.end(),
         [](const pair<std::string, Sphere::Point> &lhs, const pair<std::string, Sphere::Point> &rhs) { return lhs.second.longitude < rhs.second.longitude; });
    sort(y_stops_sorted.begin(), y_stops_sorted.end(),
         [](const pair<std::string, Sphere::Point> &lhs, const pair<std::string, Sphere::Point> &rhs) { return lhs.second.latitude < rhs.second.latitude; });

    int max_idx_x = 0, max_idx_y = 0;
    for (const auto&[stop_name, stop_point] : x_stops_sorted) {
        auto adj_it = adjacent_stops.find(stop_name);
        if (adj_it == adjacent_stops.end()) {  // if not adjacent
            x_stop_coord_idx[stop_coords.at(stop_name)] = 0;
            continue;
        }

        int max_idx = -1;
        for (const string &adj_stop : adj_it->second) {
            auto it = x_stop_coord_idx.find(stop_coords.at(adj_stop));
            if (it != x_stop_coord_idx.end() && it->second > max_idx) {
                max_idx = it->second;
            }
        }
        x_stop_coord_idx[stop_coords.at(stop_name)] = max_idx + 1;  // or (-1) + 1 == 0
        max_idx_x = max(max_idx_x, max_idx + 1);
    }

    for (const auto&[stop_name, stop_point] : y_stops_sorted) {
        auto adj_it = adjacent_stops.find(stop_name);
        if (adj_it == adjacent_stops.end()) {  // if not adjacent
            y_stop_coord_idx[stop_coords.at(stop_name)] = 0;
            continue;
        }

        int max_idx = -1;
        for (const string &adj_stop : adj_it->second) {
            auto it = y_stop_coord_idx.find(stop_coords.at(adj_stop));
            if (it != y_stop_coord_idx.end() && it->second > max_idx) {
                max_idx = it->second;
            }
        }
        y_stop_coord_idx[stop_coords.at(stop_name)] = max_idx + 1;  // or (-1) + 1 == 0
        max_idx_y = max(max_idx_y, max_idx + 1);
    }

    x_step = max_idx_x == 0 ? 0 : (renderSettings.width - 2 * renderSettings.padding) / static_cast<double>(max_idx_x);
    y_step = max_idx_y == 0 ? 0 : (renderSettings.height - 2 * renderSettings.padding) / static_cast<double>(max_idx_y);
}

PointConverterFlattenCompressRoutes::PointConverterFlattenCompressRoutes(const Serialization::PointConverterFlattenCompressRoutes &serialization_converter) {
    padding = serialization_converter.padding();
    height = serialization_converter.height();

    x_step = serialization_converter.x_step();
    y_step = serialization_converter.y_step();

    for (int i = 0; i < serialization_converter.x_stop_coord_idx_from_size(); ++i) {
        x_stop_coord_idx[{serialization_converter.x_stop_coord_idx_from(i).x(), serialization_converter.x_stop_coord_idx_from(i).y()}] = serialization_converter.x_stop_coord_idx_to(i);
    }

    for (int i = 0; i < serialization_converter.y_stop_coord_idx_from_size(); ++i) {
        y_stop_coord_idx[{serialization_converter.y_stop_coord_idx_from(i).x(), serialization_converter.y_stop_coord_idx_from(i).y()}] = serialization_converter.y_stop_coord_idx_to(i);
    }
}

Svg::Point PointConverterFlattenCompressRoutes::operator()(Sphere::Point to_convert) const {
    return {this->x_stop_coord_idx.at(to_convert) * x_step + padding, height - padding - this->y_stop_coord_idx.at(to_convert) * y_step};
}

Serialization::PointConverterFlattenCompressRoutes PointConverterFlattenCompressRoutes::SerializeConverter() const {
    Serialization::PointConverterFlattenCompressRoutes serialization_converter;

    serialization_converter.set_padding(padding);
    serialization_converter.set_height(height);
    serialization_converter.set_x_step(x_step);
    serialization_converter.set_y_step(y_step);

    for (const auto&[p, idx] : x_stop_coord_idx) {
        Serialization::Point &serialization_cur_point = *serialization_converter.add_x_stop_coord_idx_from();
        serialization_cur_point.set_x(p.latitude);
        serialization_cur_point.set_y(p.longitude);

        serialization_converter.add_x_stop_coord_idx_to(idx);
    }


    for (const auto&[p, idx] : y_stop_coord_idx) {
        Serialization::Point &serialization_cur_point = *serialization_converter.add_y_stop_coord_idx_from();
        serialization_cur_point.set_x(p.latitude);
        serialization_cur_point.set_y(p.longitude);

        serialization_converter.add_y_stop_coord_idx_to(idx);
    }

    return serialization_converter;
}


// ===========================================================================================================================================
// ========================================================= PointConverterIntermediateStops =================================================
// ===========================================================================================================================================

PointConverterIntermediateStops::PointConverterIntermediateStops() {}

PointConverterIntermediateStops::PointConverterIntermediateStops(const std::map<std::string, Sphere::Point> &stop_coords, const Descriptions::BusesDict &buses_dict) {
    unordered_set<string> bearing_stops;

    // ending stations
    for (const auto&[bus_name, desc_bus] : buses_dict) {
        bearing_stops.insert(desc_bus->stops.front());
        if (!desc_bus->is_roundtrip) { bearing_stops.insert(desc_bus->stops.at(desc_bus->stops.size() / 2)); }
    }

    // transfer stop
    unordered_map<string, vector<string>> buses_by_stop;
    for (const auto&[bus_name, desc_bus] : buses_dict) {
        for (const string &stop_in_bus : desc_bus->stops) {
            buses_by_stop[stop_in_bus].push_back(bus_name);
        }
    }
    for (const auto&[stop_name, buses_stopping] : buses_by_stop) {
        if (unordered_set(buses_stopping.begin(), buses_stopping.end()).size() > 1 || buses_stopping.size() > 2) {
            bearing_stops.insert(stop_name);
        }
    }

    // mapping
    for (const auto&[bus_name, desc_bus] : buses_dict) {
        int prev_bearing = 0;
        mapping[stop_coords.at(desc_bus->stops.at(0))] = stop_coords.at(desc_bus->stops.at(0));
        for (int next_bearing = 1; next_bearing < desc_bus->stops.size(); next_bearing++) {
            if (bearing_stops.count(desc_bus->stops.at(next_bearing))) {
                double lon_step = (stop_coords.at(desc_bus->stops.at(next_bearing)).longitude - stop_coords.at(desc_bus->stops.at(prev_bearing)).longitude) / (next_bearing - prev_bearing);
                double lat_step = (stop_coords.at(desc_bus->stops.at(next_bearing)).latitude - stop_coords.at(desc_bus->stops.at(prev_bearing)).latitude) / (next_bearing - prev_bearing);
                for (int i = prev_bearing + 1; i <= next_bearing; i++) {
                    mapping[stop_coords.at(desc_bus->stops.at(i))] = {
                            stop_coords.at(desc_bus->stops.at(prev_bearing)).latitude + lat_step * (i - prev_bearing),
                            stop_coords.at(desc_bus->stops.at(prev_bearing)).longitude + lon_step * (i - prev_bearing)
                    };
                    prev_bearing = next_bearing;
                }
            }
        }
    }
}

PointConverterIntermediateStops::PointConverterIntermediateStops(const Serialization::PointConverterIntermediateStops &serialization_converter) {
    for (int i = 0; i < serialization_converter.points_from_size(); ++i) {
        mapping[{serialization_converter.points_from(i).x(), serialization_converter.points_from(i).y()}] = {serialization_converter.points_to(i).x(), serialization_converter.points_to(i).y()};
    }
}

Sphere::Point PointConverterIntermediateStops::operator()(Sphere::Point to_convert) const {
    auto it = mapping.find(to_convert);
    if (it == mapping.end()) {
        return to_convert;  // stop without buses
    }
    return it->second;
}

Serialization::PointConverterIntermediateStops PointConverterIntermediateStops::SerializeConverter() const {
    Serialization::PointConverterIntermediateStops serialization_converter;

    for (const auto&[p_from, p_to] : mapping) {
        Serialization::Point &serialization_point_from = *serialization_converter.add_points_from();
        serialization_point_from.set_x(p_from.latitude);
        serialization_point_from.set_y(p_from.longitude);

        Serialization::Point &serialization_point_to = *serialization_converter.add_points_to();
        serialization_point_to.set_x(p_to.latitude);
        serialization_point_to.set_y(p_to.longitude);
    }

    return serialization_converter;
}


// ===========================================================================================================================================
// ========================================================= PointConverterIntermFlattenCompr =======================================================
// ===========================================================================================================================================

PointConverterIntermFlattenCompr::PointConverterIntermFlattenCompr() {}

PointConverterIntermFlattenCompr::PointConverterIntermFlattenCompr(const std::map<std::string, Sphere::Point> &stop_coords,
                                                                   const Descriptions::BusesDict &buses_dict,
                                                                   const RenderSettings &renderSettings) {
    conv_intermediate = PointConverterIntermediateStops(stop_coords, buses_dict);
    std::map<std::string, Sphere::Point> new_coords;
    transform(stop_coords.begin(), stop_coords.end(), inserter(new_coords, new_coords.end()),
              [this](const pair<std::string, Sphere::Point> &entry) { return pair(entry.first, conv_intermediate(entry.second)); });
    conv_flatten_compress = PointConverterFlattenCompressRoutes(new_coords, buses_dict, renderSettings);
}

PointConverterIntermFlattenCompr::PointConverterIntermFlattenCompr(const Serialization::PointConverterIntermFlattenCompr &serialization_converter) {
    conv_intermediate = PointConverterIntermediateStops(serialization_converter.conv_intermediate());
    conv_flatten_compress = PointConverterFlattenCompressRoutes(serialization_converter.conv_flatten_compress());
}

Svg::Point PointConverterIntermFlattenCompr::operator()(Sphere::Point to_convert) const {
    return conv_flatten_compress(conv_intermediate(to_convert));
}

Serialization::PointConverterIntermFlattenCompr PointConverterIntermFlattenCompr::SerializeConverter() const {
    Serialization::PointConverterIntermFlattenCompr serialization_converter;
    *serialization_converter.mutable_conv_intermediate() = conv_intermediate.SerializeConverter();
    *serialization_converter.mutable_conv_flatten_compress() = conv_flatten_compress.SerializeConverter();
    return serialization_converter;
}

// ===========================================================================================================================================
// ========================================================= MapRenderer =====================================================================
// ===========================================================================================================================================

MapRenderer::MapRenderer() {}

MapRenderer::MapRenderer(const Descriptions::StopsDict &stops_dict, const Descriptions::BusesDict &buses_dict, const Json::Dict &render_settings_json) {
    for (const auto&[stop_name, desc_stop] : stops_dict) {
        stops_for_render_[stop_name] = desc_stop->position;
    }
    for (const auto&[bus_name, desc_bus] : buses_dict) {
        buses_for_render_[bus_name] = {desc_bus->stops, desc_bus->is_roundtrip};
    }

    render_settings = RenderSettings(render_settings_json);
    converter = PointConverterIntermFlattenCompr(stops_for_render_, buses_dict, render_settings);

    size_t color_idx = 0;
    for (const auto&[bus_name, bus_desc] : buses_for_render_) {
        bus_line_colors[bus_name] = color_idx;
        color_idx = color_idx + 1 == render_settings.color_palette.size() ? 0 : color_idx + 1;
    }

    built_base_map_document = BuildBaseMap();
}

MapRenderer::MapRenderer(const Serialization::MapRenderer &serialization_renderer) {
    for (int bus_idx = 0; bus_idx < serialization_renderer.buses_for_render__size(); ++bus_idx) {

        vector<string> stops;
        stops.reserve(serialization_renderer.buses_for_render_(bus_idx).stops_size());
        for (int stop_idx = 0; stop_idx < serialization_renderer.buses_for_render_(bus_idx).stops_size(); ++stop_idx) {
            stops.push_back(serialization_renderer.buses_for_render_(bus_idx).stops(stop_idx));
        }

        buses_for_render_.insert(buses_for_render_.end(), {
                serialization_renderer.buses_for_render_(bus_idx).bus_name(), {
                        move(stops),
                        serialization_renderer.buses_for_render_(bus_idx).is_roundtrip()
                }
        });
    }

    for (int stop_idx = 0; stop_idx < serialization_renderer.stops_for_render__size(); ++stop_idx) {
        stops_for_render_.insert(stops_for_render_.end(), {
                serialization_renderer.stops_for_render_(stop_idx).stop_name(), {
                        serialization_renderer.stops_for_render_(stop_idx).latitude(),
                        serialization_renderer.stops_for_render_(stop_idx).longitude()
                }
        });
    }

    render_settings = RenderSettings(serialization_renderer.render_settings());

    converter = PointConverterIntermFlattenCompr(serialization_renderer.converter());

    for (int idx = 0; idx < serialization_renderer.bus_line_colors_size(); ++idx) {
        bus_line_colors[serialization_renderer.bus_line_colors(idx).bus_name()] = serialization_renderer.bus_line_colors(idx).color_idx();
    }

    built_base_map_document = BuildBaseMap();
}

void MapRenderer::RenderMapInplace(Svg::Document &doc) const {

    unordered_map<string, function<void(Svg::Document &)>> funcs{
            {"bus_lines",   [this](Svg::Document &doc) { DrawBusLines(doc); }},
            {"bus_labels",  [this](Svg::Document &doc) { DrawBusLabels(doc); }},
            {"stop_points", [this](Svg::Document &doc) { DrawStopPoints(doc); }},
            {"stop_labels", [this](Svg::Document &doc) { DrawStopLabels(doc); }},
    };

    for (const auto &layer_name : render_settings.layers) {
        auto &render_func = funcs.at(layer_name);
        render_func(doc);
    }
}

std::string MapRenderer::RenderMap() const {
    stringstream ss;
    built_base_map_document.Render(ss);
    return ss.str();
}

void MapRenderer::RenderShadowingRectInplace(Svg::Document &doc) const {
    doc.Add(  // Rect
            Svg::Rect()
                    .SetFillColor(render_settings.underlayer_color)
                    .SetUpLeft({-render_settings.outer_margin, -render_settings.outer_margin})
                    .SetDimensions({render_settings.width + 2 * render_settings.outer_margin,
                                    render_settings.height + 2 * render_settings.outer_margin})
    );
}

void MapRenderer::RenderRouteInplace(Svg::Document &doc, const std::vector<TransportRouter::RouteInfo::BusItem> &items) const {
    if (items.empty()) { return; }

    unordered_map<string, function<void(Svg::Document &, const std::vector<TransportRouter::RouteInfo::BusItem> &)>> funcs{
            {"bus_lines",   [this](Svg::Document &doc, const std::vector<TransportRouter::RouteInfo::BusItem> &route_bus_items) { DrawBusLinesInRoute(doc, route_bus_items); }},
            {"bus_labels",  [this](Svg::Document &doc, const std::vector<TransportRouter::RouteInfo::BusItem> &route_bus_items) { DrawBusLabelsInRoute(doc, route_bus_items); }},
            {"stop_points", [this](Svg::Document &doc, const std::vector<TransportRouter::RouteInfo::BusItem> &route_bus_items) { DrawStopPointsInRoute(doc, route_bus_items); }},
            {"stop_labels", [this](Svg::Document &doc, const std::vector<TransportRouter::RouteInfo::BusItem> &route_bus_items) { DrawStopLabelsInRoute(doc, route_bus_items); }},
    };

    for (const auto &layer_name : render_settings.layers) {
        auto &render_func = funcs.at(layer_name);
        render_func(doc, items);
    }
}


std::string MapRenderer::RenderRoute(const std::vector<TransportRouter::RouteInfo::BusItem> &items) const {
    Svg::Document doc = built_base_map_document;

    RenderShadowingRectInplace(doc);

    RenderRouteInplace(doc, items);

    stringstream ss;
    doc.Render(ss);
    return ss.str();
}

Serialization::MapRenderer MapRenderer::SerializeMapRenderer() const {
    Serialization::MapRenderer serialization_renderer;

    for (const auto&[bus_name, bus_desc] : buses_for_render_) {
        Serialization::BusDescForRender &serialization_bus_desc = *serialization_renderer.add_buses_for_render_();

        serialization_bus_desc.set_bus_name(bus_name);
        for (const auto &stop_name : bus_desc.stops) {
            serialization_bus_desc.add_stops(stop_name);
        }
        serialization_bus_desc.set_is_roundtrip(bus_desc.is_roundtrip);
    }

    for (const auto&[stop_name, stop_desc] : stops_for_render_) {
        Serialization::StopDescForRender &serialization_stop_desc = *serialization_renderer.add_stops_for_render_();

        serialization_stop_desc.set_stop_name(stop_name);
        serialization_stop_desc.set_latitude(stop_desc.latitude);
        serialization_stop_desc.set_longitude(stop_desc.longitude);
    }

    *serialization_renderer.mutable_render_settings() = render_settings.SerializeRenderSettings();

    *serialization_renderer.mutable_converter() = converter.SerializeConverter();

    for (const auto&[bus_name, color_idx] : bus_line_colors) {
        Serialization::BusLineColorEntry &serialization_bus_line_color_entry = *serialization_renderer.add_bus_line_colors();

        serialization_bus_line_color_entry.set_bus_name(bus_name);
        serialization_bus_line_color_entry.set_color_idx(color_idx);
    }

    return serialization_renderer;
}

Svg::Document MapRenderer::BuildBaseMap() const {
    Svg::Document doc;

    RenderMapInplace(doc);

    return doc;
}

void MapRenderer::DrawBusLines(Svg::Document &doc) const {
    for (const auto&[bus_name, bus_desc] : buses_for_render_) {
        doc.Add(DrawPolylineFromStops(bus_desc.stops.begin(), bus_desc.stops.end(), bus_line_colors.at(bus_name)));
    }
}

void MapRenderer::DrawBusLinesInRoute(Svg::Document &doc, const std::vector<TransportRouter::RouteInfo::BusItem> &route_bus_items) const {
    for (const auto &bus_route_item : route_bus_items) {
        doc.Add(DrawPolylineFromStops(
                buses_for_render_.at(bus_route_item.bus_name).stops.begin() + bus_route_item.start_stop_idx,
                buses_for_render_.at(bus_route_item.bus_name).stops.begin() + bus_route_item.finish_stop_idx + 1,
                bus_line_colors.at(bus_route_item.bus_name))
        );
    }
}

void MapRenderer::DrawBusLabelInplace(Svg::Document &doc, const string &bus_name, const string &stop_name) const {
    Svg::Text text1;
    text1.SetPoint(converter(stops_for_render_.at(stop_name)));//bus_desc.stops.at(0)
    text1.SetOffset(render_settings.bus_label_offset);
    text1.SetFontSize(render_settings.bus_label_font_size);
    text1.SetFontFamily("Verdana");
    text1.SetFontWeight("bold");
    text1.SetData(bus_name);

    text1.SetFillColor(render_settings.underlayer_color);
    text1.SetStrokeColor(render_settings.underlayer_color);
    text1.SetStrokeWidth(render_settings.underlayer_width);
    text1.SetStrokeLineCap("round");
    text1.SetStrokeLineJoin("round");
    doc.Add(text1);

    Svg::Text text2;
    text2.SetPoint(converter(stops_for_render_.at(stop_name))); //bus_desc.stops.at(0)
    text2.SetOffset(render_settings.bus_label_offset);
    text2.SetFontSize(render_settings.bus_label_font_size);
    text2.SetFontFamily("Verdana");
    text2.SetFontWeight("bold");
    text2.SetData(bus_name);

    text2.SetFillColor(render_settings.color_palette[bus_line_colors.at(bus_name)]);
    doc.Add(text2);
}

void MapRenderer::DrawBusLabels(Svg::Document &doc) const {
    for (const auto&[bus_name, bus_desc] : buses_for_render_) {
        if (bus_desc.stops.empty()) { continue; }
        DrawBusLabelInplace(doc, bus_name, bus_desc.stops.at(0));

        if (!bus_desc.is_roundtrip && bus_desc.stops.at((bus_desc.stops.size() - 1) / 2) != bus_desc.stops.at(0)) {
            size_t idx_second = (bus_desc.stops.size() - 1) / 2;
            DrawBusLabelInplace(doc, bus_name, bus_desc.stops.at(idx_second));
        }
    }
}

bool MapRenderer::CheckIfEndingStop(const BusDescForRender &bus_desc, const string &stop_name) const {
    if (bus_desc.stops.at(0) == stop_name) {
        return true;
    }
    size_t idx_middle = (bus_desc.stops.size() - 1) / 2;
    if (!bus_desc.is_roundtrip && bus_desc.stops.at(idx_middle) == stop_name) {
        return true;
    }
    return false;
}

void MapRenderer::DrawBusLabelsInRoute(Svg::Document &doc, const std::vector<TransportRouter::RouteInfo::BusItem> &route_bus_items) const {
    for (const auto &bus_item : route_bus_items) {
        const auto &bus_desc = buses_for_render_.at(bus_item.bus_name);

        const string &start_stop_name = bus_desc.stops.at(bus_item.start_stop_idx);
        if (CheckIfEndingStop(bus_desc, start_stop_name)) {
            DrawBusLabelInplace(doc, bus_item.bus_name, start_stop_name);
        }

        const string &finish_stop_name = bus_desc.stops.at(bus_item.finish_stop_idx);
        if (CheckIfEndingStop(bus_desc, finish_stop_name)) {
            DrawBusLabelInplace(doc, bus_item.bus_name, finish_stop_name);
        }
    }
}

Svg::Circle MapRenderer::DrawStopCircle(Sphere::Point coords) const {
    Svg::Circle circle;
    circle.SetCenter(converter(coords));
    circle.SetRadius(render_settings.stop_radius);
    circle.SetFillColor("white");
    return circle;
}

void MapRenderer::DrawStopPoints(Svg::Document &doc) const {
    for (const auto&[stop_name, coords] : stops_for_render_) {
        doc.Add(DrawStopCircle(coords));
    }
}

void MapRenderer::DrawStopPointsInRoute(Svg::Document &doc, const std::vector<TransportRouter::RouteInfo::BusItem> &route_bus_items) const {
    for (const auto &bus_item : route_bus_items) {
        const auto &bus_desc = buses_for_render_.at(bus_item.bus_name);

        for (int i = bus_item.start_stop_idx; i <= bus_item.finish_stop_idx; i++) {
            const string &stop_name = bus_desc.stops.at(i);
            doc.Add(DrawStopCircle(stops_for_render_.at(stop_name)));
        }
    }
}

void MapRenderer::DrawStopLabelInplace(Svg::Document &doc, Sphere::Point coords, const string &stop_name) const {
    Svg::Text text1;
    text1.SetPoint(converter(coords));
    text1.SetOffset(render_settings.stop_label_offset);
    text1.SetFontSize(render_settings.stop_label_font_size);
    text1.SetFontFamily("Verdana");
    text1.SetData(stop_name);

    text1.SetFillColor(render_settings.underlayer_color);
    text1.SetStrokeColor(render_settings.underlayer_color);
    text1.SetStrokeWidth(render_settings.underlayer_width);
    text1.SetStrokeLineCap("round");
    text1.SetStrokeLineJoin("round");
    doc.Add(text1);

    Svg::Text text2;
    text2.SetPoint(converter(coords));
    text2.SetOffset(render_settings.stop_label_offset);
    text2.SetFontSize(render_settings.stop_label_font_size);
    text2.SetFontFamily("Verdana");
    text2.SetData(stop_name);

    text2.SetFillColor("black");
    doc.Add(text2);
}

void MapRenderer::DrawStopLabels(Svg::Document &doc) const {
    for (const auto&[stop_name, coords] : stops_for_render_) {
        DrawStopLabelInplace(doc, coords, stop_name);
    }
}

void MapRenderer::DrawStopLabelsInRoute(Svg::Document &doc, const std::vector<TransportRouter::RouteInfo::BusItem> &route_bus_items) const {
    for (const auto &bus_item : route_bus_items) {
        const auto &bus_desc = buses_for_render_.at(bus_item.bus_name);
        const string &stop_name = bus_desc.stops.at(bus_item.start_stop_idx);

        DrawStopLabelInplace(doc, stops_for_render_.at(stop_name), stop_name);
    }

    // last
    const auto &bus_item = route_bus_items.back();
    const auto &bus_desc = buses_for_render_.at(bus_item.bus_name);
    const string &stop_name = bus_desc.stops.at(bus_item.finish_stop_idx);

    DrawStopLabelInplace(doc, stops_for_render_.at(stop_name), stop_name);
}