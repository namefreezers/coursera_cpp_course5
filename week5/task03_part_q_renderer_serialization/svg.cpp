#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include "svg.h"

using namespace std;

namespace Svg {

    RgbA::RgbA(std::initializer_list<uint8_t> in_list) {
        auto it = begin(in_list);
        red = *(it++);
        green = *(it++);
        blue = *(it++);
    }

    RgbA::RgbA(const vector<Json::Node> &colors) {
        red = colors.at(0).AsInt();
        green = colors.at(1).AsInt();
        blue = colors.at(2).AsInt();
        if (colors.size() > 3) {
            alpha = colors.at(3).AsDouble();
        }
    }


    // =============================== Color ===================================

    Color::Color() {}

    Color::Color(const std::string &color_to_init) : opt_color_str(color_to_init) {}

    Color::Color(const char *color_to_init_c_str) : opt_color_str(color_to_init_c_str) {}

    Color::Color(const RgbA &rgb) {
        stringstream ss;
        ss << (rgb.alpha ? "rgba(" : "rgb(") << unsigned(rgb.red) << "," << unsigned(rgb.green) << "," << unsigned(rgb.blue);
        if (rgb.alpha) {
            ss << "," << *rgb.alpha;
        }
        ss << ")";
        opt_color_str = ss.str();
    }

    Color::Color(const Json::Node &render_settings_json) : Color(holds_alternative<std::string>(render_settings_json.GetBase()) ?
                                                                 Color(render_settings_json.AsString()) :
                                                                 Color(RgbA(render_settings_json.AsArray()))) {}

    Color::Color(const Serialization::Color &serialization_color) : opt_color_str(serialization_color.present() ?
                                                                                  std::optional<std::string>(serialization_color.color()) :
                                                                                  nullopt) {}

    Color::operator string() const {
        return opt_color_str ? *opt_color_str : "none";
    }

    Color NoneColor;

    // =============================== Circle ==================================

    Circle &Circle::SetCenter(Point p) {
        center_cx_cy = p;
        return *this;
    }

    Circle &Circle::SetRadius(double r) {
        radius_r = r;
        return *this;
    }

    Circle::operator std::string() const {
        std::stringstream ss;
        ss << "<circle ";
        ss << "cx=\"" << center_cx_cy.x << "\" cy=\"" << center_cx_cy.y << "\" ";
        ss << "r=\"" << radius_r << "\" ";
        ss << get_base_params_xml();
        ss << "/>";
        return ss.str();
    }

    // =============================== Polyline ================================

    Polyline &Polyline::AddPoint(Point p) {
        points.push_back(p);
        return *this;
    }

    Polyline::operator std::string() const {
        std::stringstream ss;
        ss << "<polyline ";
        ss << "points=\"";
        for (const Point p : points) {
            ss << p.x << "," << p.y << " ";
        }
        ss << "\" ";
        ss << get_base_params_xml();
        ss << "/>";
        return ss.str();
    }

    // =============================== Text ====================================

    Text &Text::SetPoint(Point p) {
        x_y = p;
        return *this;
    }

    Text &Text::SetOffset(Point p) {
        dx_dy = p;
        return *this;
    }

    Text &Text::SetFontSize(uint32_t sz) {
        font_size = sz;
        return *this;
    }

    Text &Text::SetFontFamily(const string &font) {
        font_family = font;
        return *this;
    }

    Text &Text::SetFontWeight(const string &font_weight_) {
        font_weight = font_weight_;
        return *this;
    }

    Text &Text::SetData(const string &data) {
        text = data;
        return *this;
    }

    Text::operator std::string() const {
        std::stringstream ss;
        ss << "<text ";
        ss << "x=\"" << x_y.x << "\" y=\"" << x_y.y << "\" ";
        ss << "dx=\"" << dx_dy.x << "\" dy=\"" << dx_dy.y << "\" ";
        ss << "font-size=\"" << font_size << "\" ";
        if (font_family) {
            ss << "font-family=\"" << *font_family << "\" ";
        }
        if (font_weight) {
            ss << "font-weight=\"" << *font_weight << "\" ";
        }
        ss << get_base_params_xml();
        ss << ">";
        ss << text;
        ss << "</text>";
        return ss.str();
    }

    // =============================== Rect ====================================

    Rect &Rect::SetUpLeft(Point p) {
        center_cx_cy = p;
        return *this;
    }

    Rect &Rect::SetDimensions(Point p) {
        dimensions_w_h = p;
        return *this;
    }

    Rect::operator std::string() const {
        std::stringstream ss;
        ss << "<rect ";
        ss << "x=\"" << center_cx_cy.x << "\" y=\"" << center_cx_cy.y << "\" ";
        ss << "width=\"" << dimensions_w_h.x << "\" height=\"" << dimensions_w_h.y << "\" ";
        ss << get_base_params_xml();
        ss << "/>";
        return ss.str();
    }

    // =============================== Document ================================

    void Document::Render(std::ostream &out) const {
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << std::endl;
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">" << std::endl;
        for (const auto &node : svg_objects) {  // TODO: const unique_ptr<SvgObject> &node
            out << std::visit([](const auto &node) {
                                  return std::string(node);
                              },
                              node) << std::endl;
        }
        out << "</svg>" << std::endl;
    }
}


//int main() {
//    Svg::Document svg;
//
//    svg.Add(
//            Svg::Polyline{}
//                    .SetStrokeColor(Svg::RgbA{140, 198, 63})  // soft green
//                    .SetStrokeWidth(16)
//                    .SetStrokeLineCap("round")
//                    .AddPoint({50, 50})
//                    .AddPoint({250, 250})
//    );
//
//    for (const auto point : {Svg::Point{50, 50}, Svg::Point{250, 250}}) {
//        svg.Add(
//                Svg::Circle{}
//                        .SetFillColor("white")
//                        .SetRadius(6)
//                        .SetUpLeft(point)
//        );
//    }
//
//    svg.Add(
//            Svg::Text{}
//                    .SetPoint({50, 50})
//                    .SetOffset({10, -10})
//                    .SetFontSize(20)
//                    .SetFontFamily("Verdana")
//                    .SetFillColor("black")
//                    .SetData("C")
//    );
//    svg.Add(
//            Svg::Text{}
//                    .SetPoint({250, 250})
//                    .SetOffset({10, -10})
//                    .SetFontSize(20)
//                    .SetFontFamily("Verdana")
//                    .SetFillColor("black")
//                    .SetData("C++")
//    );
//
//    svg.Render(std::cout);
//
//}
