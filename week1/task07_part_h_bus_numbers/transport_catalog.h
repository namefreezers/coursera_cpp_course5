#pragma once

#include "descriptions.h"
#include "json.h"
#include "svg.h"
#include "transport_router.h"
#include "utils.h"

#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

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
    TransportCatalog(std::vector<Descriptions::InputQuery> data, const Json::Dict &routing_settings_json, const Json::Dict &render_settings_json);

    const Stop *GetStop(const std::string &name) const;

    const Bus *GetBus(const std::string &name) const;

    std::optional<TransportRouter::RouteInfo> FindRoute(const std::string &stop_from, const std::string &stop_to) const;

    std::string RenderMap() const;

private:
    struct RenderSettings {
        RenderSettings();

        explicit RenderSettings(const Json::Dict &render_settings_json);

        double width, height;
        double padding;
        double stop_radius;
        double line_width;
        int stop_label_font_size;
        Svg::Point stop_label_offset;
        Svg::Color underlayer_color;
        double underlayer_width;
        std::vector<Svg::Color> color_palette;
    };

    class PointConverter {
    public:
        explicit PointConverter(const std::map<std::string, Sphere::Point> &stop_coords, const RenderSettings &renderSettings);

        Svg::Point operator()(Sphere::Point to_convert) const;

    private:
        double min_lon;
        double max_lat;
        double zoom_coef;
        double padding;
    };

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

    std::map<std::string, std::vector<std::string>> bus_stops_;
    std::map<std::string, Sphere::Point> stop_coords_;
    RenderSettings render_settings;
    std::unique_ptr<TransportRouter> router_;
};
