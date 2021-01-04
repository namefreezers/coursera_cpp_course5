#pragma once

#include "descriptions.h"
#include "svg.h"
#include "transport_router.h"

struct RenderSettings {
    RenderSettings();

    explicit RenderSettings(const Json::Dict &render_settings_json);

    double width;
    double height;
    double padding;
    double stop_radius;
    double line_width;
    int stop_label_font_size;
    Svg::Point stop_label_offset;
    Svg::Color underlayer_color;
    double underlayer_width;
    std::vector<Svg::Color> color_palette;
    int bus_label_font_size;
    Svg::Point bus_label_offset;
    std::vector<std::string> layers;
    double outer_margin;
};

struct BusDescForRender {
    std::vector<std::string> stops;
    bool is_roundtrip;
};

class PointConverter {
public:
    PointConverter();

    explicit PointConverter(const std::map<std::string, Sphere::Point> &stop_coords, const RenderSettings &renderSettings);

    Svg::Point operator()(Sphere::Point to_convert) const;

private:
    double min_lon;
    double max_lat;
    double zoom_coef;
    double padding;
};

class PointConverterFlattened {
public:
    PointConverterFlattened();

    explicit PointConverterFlattened(const std::map<std::string, Sphere::Point> &stop_coords, const RenderSettings &renderSettings);

    Svg::Point operator()(Sphere::Point to_convert) const;

private:
    double padding;
    double height;

    double x_step;
    double y_step;
    std::vector<double> x_coords_sorted, y_coords_sorted;
};

class PointConverterFlattenCompress {
public:
    PointConverterFlattenCompress();

    explicit PointConverterFlattenCompress(const std::map<std::string, Sphere::Point> &stop_coords, const Descriptions::BusesDict &buses_dict, const RenderSettings &renderSettings);

    Svg::Point operator()(Sphere::Point to_convert) const;

private:
    double padding;
    double height;

    double x_step;
    double y_step;
    std::vector<double> x_coords_sorted, y_coords_sorted;
};

class PointConverterFlattenCompressRoutes {
public:
    PointConverterFlattenCompressRoutes();

    explicit PointConverterFlattenCompressRoutes(const std::map<std::string, Sphere::Point> &stop_coords, const Descriptions::BusesDict &buses_dict, const RenderSettings &renderSettings);

    Svg::Point operator()(Sphere::Point to_convert) const;

private:
    double padding;
    double height;

    double x_step;
    double y_step;

    std::unordered_map<Sphere::Point, int, Sphere::PointHash> x_stop_coord_idx, y_stop_coord_idx;
};

class PointConverterIntermediateStops {
public:
    PointConverterIntermediateStops();

    explicit PointConverterIntermediateStops(const std::map<std::string, Sphere::Point> &stop_coords, const Descriptions::BusesDict &buses_dict);

    Sphere::Point operator()(Sphere::Point to_convert) const;

private:
    std::unordered_map<Sphere::Point, Sphere::Point, Sphere::PointHash> mapping;
};

class PointConverterIntermFlattenCompr {
public:
    PointConverterIntermFlattenCompr();

    explicit PointConverterIntermFlattenCompr(const std::map<std::string, Sphere::Point> &stop_coords, const Descriptions::BusesDict &buses_dict, const RenderSettings &renderSettings);

    Svg::Point operator()(Sphere::Point to_convert) const;

private:
    PointConverterIntermediateStops conv_intermediate;
    PointConverterFlattenCompressRoutes conv_flatten_compress;
};

class MapRenderer {
public:
    MapRenderer();

    MapRenderer(const Descriptions::StopsDict &stops_dict, const Descriptions::BusesDict &buses_dict, const Json::Dict &render_settings_json);

    std::string RenderMap() const;

    std::string RenderRoute(const std::vector<TransportRouter::RouteInfo::BusItem>& items) const;



private:
    void RenderMapInplace(Svg::Document& doc) const;

    void RenderShadowingRectInplace(Svg::Document& doc) const;

    void RenderRouteInplace(Svg::Document& doc, const std::vector<TransportRouter::RouteInfo::BusItem>& route_bus_items) const;

    template <typename It>
    Svg::Polyline DrawPolylineFromStops(It it_begin, It it_end, int color_idx) const {
        Svg::Polyline polyline;
        for (auto stop_it = it_begin; stop_it != it_end; stop_it++) {
            polyline.AddPoint(converter(stops_for_render_.at(*stop_it)));
        }
        polyline.SetStrokeColor(render_settings.color_palette[color_idx]);
        polyline.SetStrokeWidth(render_settings.line_width);
        polyline.SetStrokeLineCap("round");
        polyline.SetStrokeLineJoin("round");

        return polyline;
    }


    void DrawBusLines(Svg::Document &doc) const;

    void DrawBusLinesInRoute(Svg::Document &doc, const std::vector<TransportRouter::RouteInfo::BusItem>& route_bus_items) const;

    void DrawBusLabelInplace(Svg::Document &doc, const std::string& bus_name, const std::string& stop_name) const;

    void DrawBusLabels(Svg::Document &doc) const;

    bool CheckIfEndingStop(const BusDescForRender& bus_desc, const std::string& stop_name) const;

    void DrawBusLabelsInRoute(Svg::Document &doc, const std::vector<TransportRouter::RouteInfo::BusItem>& route_bus_items) const;

    Svg::Circle DrawStopCircle(Sphere::Point coords) const;

    void DrawStopPoints(Svg::Document &doc) const;

    void DrawStopPointsInRoute(Svg::Document &doc, const std::vector<TransportRouter::RouteInfo::BusItem>& route_bus_items) const;

    void DrawStopLabelInplace(Svg::Document &doc, Sphere::Point coords, const std::string& stop_name) const;

    void DrawStopLabels(Svg::Document &doc) const;

    void DrawStopLabelsInRoute(Svg::Document &doc, const std::vector<TransportRouter::RouteInfo::BusItem>& route_bus_items) const;

    std::map<std::string, BusDescForRender> buses_for_render_;
    std::map<std::string, Sphere::Point> stops_for_render_;
    RenderSettings render_settings;
    PointConverterIntermFlattenCompr converter;
    std::unordered_map<std::string, int> bus_line_colors;
};