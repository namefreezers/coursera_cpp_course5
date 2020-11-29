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
}

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


PointConverterFlattened_2::PointConverterFlattened_2() {}

bool is_next_stop_adjacent(const vector<string> &cur_coord_stops, const string &next_stop, const unordered_map<string, unordered_set<string>> &adjacent_stops) {
    auto it_next_stop_adjacent = adjacent_stops.find(next_stop);
    if (it_next_stop_adjacent == adjacent_stops.end()) {  // if stop not in bus -> not adjacent with any
        return false;
    }
    const unordered_set<string>& next_stop_adjacent = it_next_stop_adjacent->second;

    // if adjacent with any of
    return any_of(
            cur_coord_stops.begin(),
            cur_coord_stops.end(),
            [&next_stop_adjacent](const string &cur_coord_stop) {
                return next_stop_adjacent.count(cur_coord_stop);
            }
    );
}

PointConverterFlattened_2::PointConverterFlattened_2(const std::map<std::string, Sphere::Point> &stop_coords, const Descriptions::BusesDict &buses_dict, const RenderSettings &renderSettings) {
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

Svg::Point PointConverterFlattened_2::operator()(Sphere::Point to_convert) const {
    int idx_x = prev(upper_bound(x_coords_sorted.begin(), x_coords_sorted.end(), to_convert.longitude)) - x_coords_sorted.begin();
    int idx_y = prev(upper_bound(y_coords_sorted.begin(), y_coords_sorted.end(), to_convert.latitude)) - y_coords_sorted.begin();
    return {idx_x * x_step + padding, height - padding - idx_y * y_step};
}


MapRenderer::MapRenderer() {}

MapRenderer::MapRenderer(const Descriptions::StopsDict &stops_dict, const Descriptions::BusesDict &buses_dict, const Json::Dict &render_settings_json) {
    for (const auto&[stop_name, desc_stop] : stops_dict) {
        stops_for_render_[stop_name] = desc_stop->position;
    }
    for (const auto&[bus_name, desc_bus] : buses_dict) {
        buses_for_render_[bus_name] = {desc_bus->stops, desc_bus->is_roundtrip};
    }

    render_settings = RenderSettings(render_settings_json);
    converter = PointConverterFlattened_2(stops_for_render_, buses_dict, render_settings);
}

std::string MapRenderer::RenderMap() const {
    Svg::Document doc;

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

    stringstream ss;
    doc.Render(ss);
    return ss.str();
}

void MapRenderer::DrawBusLines(Svg::Document &doc) const {
    size_t color_idx = 0;
    for (const auto&[bus_name, bus_desc] : buses_for_render_) {
        Svg::Polyline polyline;
        for (const auto &stop : bus_desc.stops) {
            polyline.AddPoint(converter(stops_for_render_.at(stop)));
        }
        polyline.SetStrokeColor(render_settings.color_palette[color_idx]);
        polyline.SetStrokeWidth(render_settings.line_width);
        polyline.SetStrokeLineCap("round");
        polyline.SetStrokeLineJoin("round");

        doc.Add(polyline);
        color_idx = color_idx + 1 == render_settings.color_palette.size() ? 0 : color_idx + 1;
    }
}

void MapRenderer::DrawBusLabels(Svg::Document &doc) const {
    size_t color_idx = 0;
    for (const auto&[bus_name, bus_desc] : buses_for_render_) {
        if (bus_desc.stops.empty()) { continue; }

        Svg::Text text1;
        text1.SetPoint(converter(stops_for_render_.at(bus_desc.stops.at(0))));
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
        text2.SetPoint(converter(stops_for_render_.at(bus_desc.stops.at(0))));
        text2.SetOffset(render_settings.bus_label_offset);
        text2.SetFontSize(render_settings.bus_label_font_size);
        text2.SetFontFamily("Verdana");
        text2.SetFontWeight("bold");
        text2.SetData(bus_name);

        text2.SetFillColor(render_settings.color_palette[color_idx]);
        doc.Add(text2);

        if (!bus_desc.is_roundtrip && bus_desc.stops.at((bus_desc.stops.size() - 1) / 2) != bus_desc.stops.at(0)) {
            size_t idx_second = (bus_desc.stops.size() - 1) / 2;

            Svg::Text text3;
            text3.SetPoint(converter(stops_for_render_.at(bus_desc.stops.at(idx_second))));
            text3.SetOffset(render_settings.bus_label_offset);
            text3.SetFontSize(render_settings.bus_label_font_size);
            text3.SetFontFamily("Verdana");
            text3.SetFontWeight("bold");
            text3.SetData(bus_name);

            text3.SetFillColor(render_settings.underlayer_color);
            text3.SetStrokeColor(render_settings.underlayer_color);
            text3.SetStrokeWidth(render_settings.underlayer_width);
            text3.SetStrokeLineCap("round");
            text3.SetStrokeLineJoin("round");
            doc.Add(text3);

            Svg::Text text4;
            text4.SetPoint(converter(stops_for_render_.at(bus_desc.stops.at(idx_second))));
            text4.SetOffset(render_settings.bus_label_offset);
            text4.SetFontSize(render_settings.bus_label_font_size);
            text4.SetFontFamily("Verdana");
            text4.SetFontWeight("bold");
            text4.SetData(bus_name);

            text4.SetFillColor(render_settings.color_palette[color_idx]);
            doc.Add(text4);
        }

        color_idx = color_idx + 1 == render_settings.color_palette.size() ? 0 : color_idx + 1;
    }
}

void MapRenderer::DrawStopPoints(Svg::Document &doc) const {
    for (const auto&[stop_name, coords] : stops_for_render_) {
        Svg::Circle circle;
        circle.SetCenter(converter(coords));
        circle.SetRadius(render_settings.stop_radius);
        circle.SetFillColor("white");
        doc.Add(circle);
    }
}

void MapRenderer::DrawStopLabels(Svg::Document &doc) const {
    for (const auto&[stop_name, coords] : stops_for_render_) {
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
}
