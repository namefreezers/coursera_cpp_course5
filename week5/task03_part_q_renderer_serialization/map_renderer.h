#pragma once

#include "descriptions.h"
#include "svg.h"
#include "transport_router.h"

#include "transport_catalog.pb.h"

struct RenderSettings {
    RenderSettings();

    explicit RenderSettings(const Json::Dict &render_settings_json);

    explicit RenderSettings(const Serialization::RenderSettings &serialization_render_settings) {
        width = serialization_render_settings.width();
        height = serialization_render_settings.height();
        padding = serialization_render_settings.padding();
        stop_radius = serialization_render_settings.stop_radius();
        line_width = serialization_render_settings.line_width();
        stop_label_font_size = serialization_render_settings.stop_label_font_size();
        stop_label_offset = {serialization_render_settings.stop_label_offset().x(), serialization_render_settings.stop_label_offset().y()};
        underlayer_color = Svg::Color(serialization_render_settings.underlayer_color());
        underlayer_width = serialization_render_settings.underlayer_width();
        color_palette.reserve(serialization_render_settings.color_palette_size());
        for (int color_palette_idx = 0; color_palette_idx < serialization_render_settings.color_palette_size(); ++color_palette_idx) {
            color_palette.emplace_back(serialization_render_settings.color_palette(color_palette_idx));
        }
        bus_label_font_size = serialization_render_settings.bus_label_font_size();
        bus_label_offset = {serialization_render_settings.bus_label_offset().x(), serialization_render_settings.bus_label_offset().y()};
        layers.reserve(serialization_render_settings.layers_size());
        for (int layers_idx = 0; layers_idx < serialization_render_settings.layers_size(); ++layers_idx) {
            layers.push_back(serialization_render_settings.layers(layers_idx));
        }
        outer_margin = serialization_render_settings.outer_margin();
    }

    Serialization::RenderSettings SerializeRenderSettings() const {
        Serialization::RenderSettings serialization_render_settings;
        serialization_render_settings.set_width(width);
        serialization_render_settings.set_height(height);
        serialization_render_settings.set_padding(padding);
        serialization_render_settings.set_stop_radius(stop_radius);
        serialization_render_settings.set_line_width(line_width);
        serialization_render_settings.set_stop_label_font_size(stop_label_font_size);
        serialization_render_settings.mutable_stop_label_offset()->set_x(stop_label_offset.x);
        serialization_render_settings.mutable_stop_label_offset()->set_y(stop_label_offset.y);

        serialization_render_settings.mutable_underlayer_color()->set_present(underlayer_color.get_underlying_value().has_value());
        if (underlayer_color.get_underlying_value().has_value()) {
            serialization_render_settings.mutable_underlayer_color()->set_color(*underlayer_color.get_underlying_value());
        }

        serialization_render_settings.set_underlayer_width(underlayer_width);

        for (const auto &cur_color : color_palette) {
            Serialization::Color& serialization_cur_color = *serialization_render_settings.add_color_palette();
            serialization_cur_color.set_present(cur_color.get_underlying_value().has_value());
            if (cur_color.get_underlying_value().has_value()) {
                serialization_cur_color.set_color(*cur_color.get_underlying_value());
            }
        }

        serialization_render_settings.set_bus_label_font_size(bus_label_font_size);
        serialization_render_settings.mutable_bus_label_offset()->set_x(bus_label_offset.x);
        serialization_render_settings.mutable_bus_label_offset()->set_y(bus_label_offset.y);

        for (const auto &cur_layer : layers) {
            serialization_render_settings.add_layers(cur_layer);
        }
        serialization_render_settings.set_outer_margin(outer_margin);

        return serialization_render_settings;
    }

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

    explicit PointConverterFlattenCompressRoutes(const Serialization::PointConverterFlattenCompressRoutes& serialization_converter);

    Svg::Point operator()(Sphere::Point to_convert) const;

    Serialization::PointConverterFlattenCompressRoutes SerializeConverter() const;

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

    explicit PointConverterIntermediateStops(const Serialization::PointConverterIntermediateStops &serialization_converter);

    Sphere::Point operator()(Sphere::Point to_convert) const;

    Serialization::PointConverterIntermediateStops SerializeConverter() const;

private:
    std::unordered_map<Sphere::Point, Sphere::Point, Sphere::PointHash> mapping;
};

class PointConverterIntermFlattenCompr {
public:
    PointConverterIntermFlattenCompr();

    explicit PointConverterIntermFlattenCompr(const std::map<std::string, Sphere::Point> &stop_coords, const Descriptions::BusesDict &buses_dict, const RenderSettings &renderSettings);

    explicit PointConverterIntermFlattenCompr(const Serialization::PointConverterIntermFlattenCompr& serialization_converter);

    Svg::Point operator()(Sphere::Point to_convert) const;

    Serialization::PointConverterIntermFlattenCompr SerializeConverter() const;

private:
    PointConverterIntermediateStops conv_intermediate;
    PointConverterFlattenCompressRoutes conv_flatten_compress;
};

class MapRenderer {
public:
    MapRenderer();

    MapRenderer(const Descriptions::StopsDict &stops_dict, const Descriptions::BusesDict &buses_dict, const Json::Dict &render_settings_json);

    MapRenderer(const Serialization::MapRenderer &serialization_renderer);

    std::string RenderMap() const;

    std::string RenderRoute(const std::vector<TransportRouter::RouteInfo::BusItem> &items) const;

    Serialization::MapRenderer SerializeMapRenderer() const;

private:
    Svg::Document BuildBaseMap() const;

    void RenderMapInplace(Svg::Document &doc) const;

    void RenderShadowingRectInplace(Svg::Document &doc) const;

    void RenderRouteInplace(Svg::Document &doc, const std::vector<TransportRouter::RouteInfo::BusItem> &route_bus_items) const;

    template<typename It>
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

    void DrawBusLinesInRoute(Svg::Document &doc, const std::vector<TransportRouter::RouteInfo::BusItem> &route_bus_items) const;

    void DrawBusLabelInplace(Svg::Document &doc, const std::string &bus_name, const std::string &stop_name) const;

    void DrawBusLabels(Svg::Document &doc) const;

    bool CheckIfEndingStop(const BusDescForRender &bus_desc, const std::string &stop_name) const;

    void DrawBusLabelsInRoute(Svg::Document &doc, const std::vector<TransportRouter::RouteInfo::BusItem> &route_bus_items) const;

    Svg::Circle DrawStopCircle(Sphere::Point coords) const;

    void DrawStopPoints(Svg::Document &doc) const;

    void DrawStopPointsInRoute(Svg::Document &doc, const std::vector<TransportRouter::RouteInfo::BusItem> &route_bus_items) const;

    void DrawStopLabelInplace(Svg::Document &doc, Sphere::Point coords, const std::string &stop_name) const;

    void DrawStopLabels(Svg::Document &doc) const;

    void DrawStopLabelsInRoute(Svg::Document &doc, const std::vector<TransportRouter::RouteInfo::BusItem> &route_bus_items) const;

    std::map<std::string, BusDescForRender> buses_for_render_;
    std::map<std::string, Sphere::Point> stops_for_render_;
    RenderSettings render_settings;
    PointConverterIntermFlattenCompr converter;
    std::unordered_map<std::string, int> bus_line_colors;

    Svg::Document built_base_map_document;
};