#include "transport_catalog.h"
#include "svg.h"

#include <algorithm>
#include <sstream>

using namespace std;

TransportCatalog::TransportCatalog(vector<Descriptions::InputQuery> data, const Json::Dict &routing_settings_json, const Json::Dict &render_settings_json) {
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


    for (const auto&[stop_name, desc_stop] : stops_dict) {
        stops_for_render_[stop_name] = desc_stop->position;
    }
    for (const auto&[bus_name, desc_bus] : buses_dict) {
        buses_for_render_[bus_name] = {desc_bus->stops, desc_bus->is_roundtrip};
    }

    render_settings = RenderSettings(render_settings_json);

    router_ = make_unique<TransportRouter>(stops_dict, buses_dict, routing_settings_json);
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
    PointConverter converter(stops_for_render_, render_settings);
    Svg::Document doc;

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

    color_idx = 0;
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

    for (const auto&[stop_name, coords] : stops_for_render_) {
        Svg::Circle circle;
        circle.SetCenter(converter(coords));
        circle.SetRadius(render_settings.stop_radius);
        circle.SetFillColor("white");
        doc.Add(circle);
    }

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

    stringstream ss;
    doc.Render(ss);
    return ss.str();
}

TransportCatalog::RenderSettings::RenderSettings() {}

TransportCatalog::RenderSettings::RenderSettings(const Json::Dict &render_settings_json) {
    width = render_settings_json.at("width").AsDouble(), height = render_settings_json.at("height").AsDouble();
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
}

TransportCatalog::PointConverter::PointConverter(const std::map<std::string, Sphere::Point> &stop_coords, const RenderSettings &renderSettings) {
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

Svg::Point TransportCatalog::PointConverter::operator()(Sphere::Point to_convert) const {
    return {(to_convert.longitude - min_lon) * zoom_coef + padding, (max_lat - to_convert.latitude) * zoom_coef + padding};
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
