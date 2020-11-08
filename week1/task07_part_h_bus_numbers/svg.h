#pragma once

#include <cstdint>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

#include "json.h"

namespace Svg {
    struct Point {
        double x, y;
    };

    struct RgbA {
        RgbA(std::initializer_list<uint8_t> c);

        RgbA(const std::vector<Json::Node> &colors);

        uint8_t red, green, blue;
        std::optional<double> alpha = std::nullopt;
    };

    class Color {
    public:
        Color();

        Color(const std::string &color_to_init);

        Color(const char *color_to_init_c_str);

        Color(const RgbA &rgb);

        Color(const Json::Node &render_settings_json);

        explicit operator std::string() const;

    private:
        std::optional<std::string> opt_color_str;
    };

    template<typename Derived_T>
    class SvgObject {
    public:
        virtual ~SvgObject() = default;

        Derived_T &SetFillColor(const Color &fill_color) {
            fill = fill_color;
            return static_cast<Derived_T &>(*this);
        }

        Derived_T &SetStrokeColor(const Color &stroke_color) {
            stroke = stroke_color;
            return static_cast<Derived_T &>(*this);
        }

        Derived_T &SetStrokeWidth(double stroke_width_) {
            stroke_width = stroke_width_;
            return static_cast<Derived_T &>(*this);
        }

        Derived_T &SetStrokeLineCap(const std::string &stroke_linecap_) {
            stroke_linecap = stroke_linecap_;
            return static_cast<Derived_T &>(*this);
        }

        Derived_T &SetStrokeLineJoin(const std::string &stroke_linejoin_) {
            stroke_linejoin = stroke_linejoin_;
            return static_cast<Derived_T &>(*this);
        }

        std::string get_base_params_xml() const {
            std::stringstream ss;
            ss << "fill=\"" + std::string(fill) + "\" ";
            ss << "stroke=\"" + std::string(stroke) + "\" ";
            ss << "stroke-width=\"" + std::to_string(stroke_width) + "\" ";
            if (stroke_linecap) { ss << "stroke-linecap=\"" + *stroke_linecap + "\" "; }
            if (stroke_linejoin) { ss << "stroke-linejoin=\"" + *stroke_linejoin + "\" "; }
            return ss.str();
        }

        virtual explicit operator std::string() const = 0;

    private:
        Color fill;
        Color stroke;
        double stroke_width = 1.0;
        std::optional<std::string> stroke_linecap = std::nullopt;
        std::optional<std::string> stroke_linejoin = std::nullopt;

    };

    class Circle : public SvgObject<Circle> {
    public:
        Circle &SetCenter(Point p);

        Circle &SetRadius(double r);

        explicit operator std::string() const override;

    private:
        Point center_cx_cy{0.0, 0.0};
        double radius_r = 1.0;
    };

    class Polyline : public SvgObject<Polyline> {
    public:
        Polyline &AddPoint(Point p);

        explicit operator std::string() const override;

    private:
        std::vector<Point> points;
    };

    class Text : public SvgObject<Text> {
    public:
        Text &SetPoint(Point p);

        Text &SetOffset(Point p);

        Text &SetFontSize(uint32_t sz);

        Text &SetFontFamily(const std::string &font);

        Text &SetData(const std::string &data);

        explicit operator std::string() const override;

    private:
        Point x_y{0.0, 0.0}, dx_dy{0.0, 0.0};
        uint32_t font_size = 1;
        std::optional<std::string> font_family = std::nullopt;
        std::string text = "";
    };

    class Document {
    public:
        template<typename SvgObject_T>
        void Add(SvgObject_T svg_object) {
            svg_objects.emplace_back(std::move(svg_object));
        }

        void Render(std::ostream &out);

    private:
        std::vector<std::variant<Circle, Polyline, Text>> svg_objects;
    };


}