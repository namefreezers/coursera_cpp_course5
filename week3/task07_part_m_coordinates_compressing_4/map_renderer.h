#pragma once

#include "descriptions.h"
#include "svg.h"

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

private:

    void DrawBusLines(Svg::Document &doc) const;

    void DrawBusLabels(Svg::Document &doc) const;

    void DrawStopPoints(Svg::Document &doc) const;

    void DrawStopLabels(Svg::Document &doc) const;

    std::map<std::string, BusDescForRender> buses_for_render_;
    std::map<std::string, Sphere::Point> stops_for_render_;
    RenderSettings render_settings;
    PointConverterIntermFlattenCompr converter;
};