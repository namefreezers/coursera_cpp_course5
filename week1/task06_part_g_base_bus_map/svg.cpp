#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

using namespace std;

namespace Svg {
    struct Point {
        double x, y;
    };

    struct Rgb {
        uint8_t red, green, blue;
    };

    class Color {
    public:
        Color() {}

        Color(const string &color_to_init) : opt_color_str(color_to_init) {}

        Color(const char *color_to_init_c_str) : opt_color_str(color_to_init_c_str) {}

        Color(const Rgb &rgb) {
            stringstream ss;
            ss << "rgb(" << unsigned(rgb.red) << "," << unsigned(rgb.green) << "," << unsigned(rgb.blue) << ")";
            opt_color_str = ss.str();
        }

        explicit operator string() const {
            return opt_color_str ? *opt_color_str : "none";
        }

    private:
        optional<string> opt_color_str;
    };

    Color NoneColor;

    template<typename Derived_T>
    class SvgObject {
    public:
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

        Derived_T &SetStrokeLineCap(const string &stroke_linecap_) {
            stroke_linecap = stroke_linecap_;
            return static_cast<Derived_T &>(*this);
        }

        Derived_T &SetStrokeLineJoin(const string &stroke_linejoin_) {
            stroke_linejoin = stroke_linejoin_;
            return static_cast<Derived_T &>(*this);
        }

        string get_base_params_xml() const {
            stringstream ss;
            ss << "fill=\"" + string(fill) + "\" ";
            ss << "stroke=\"" + string(stroke) + "\" ";
            ss << "stroke-width=\"" + to_string(stroke_width) + "\" ";
            if (stroke_linecap) { ss << "stroke-linecap=\"" + *stroke_linecap + "\" "; }
            if (stroke_linejoin) { ss << "stroke-linejoin=\"" + *stroke_linejoin + "\" "; }
            return ss.str();
        }

        virtual explicit operator string() const = 0;

    private:
        Color fill;
        Color stroke;
        double stroke_width = 1.0;
        optional<string> stroke_linecap = nullopt;
        optional<string> stroke_linejoin = nullopt;

    };

    class Circle : public SvgObject<Circle> {
    public:
        Circle &SetCenter(Point p) {
            center_cx_cy = p;
            return *this;
        }

        Circle &SetRadius(double r) {
            radius_r = r;
            return *this;
        }

        explicit operator string() const override {
            stringstream ss;
            ss << "<circle ";
            ss << get_base_params_xml();
            ss << "cx=\"" << center_cx_cy.x << "\" cy=\"" << center_cx_cy.y << "\" ";
            ss << "r=\"" << radius_r << "\" ";
            ss << "/>";
            return ss.str();
        }

    private:
        Point center_cx_cy{0.0, 0.0};
        double radius_r = 1.0;
    };

    class Polyline : public SvgObject<Polyline> {
    public:
        Polyline &AddPoint(Point p) {
            points.push_back(p);
            return *this;
        }

        explicit operator string() const override {
            stringstream ss;
            ss << "<polyline ";
            ss << get_base_params_xml();
            ss << "points=\"";
            for (const Point p : points) {
                ss << p.x << "," << p.y << " ";
            }
            ss << "\" />";
            return ss.str();
        }

    private:
        vector<Point> points;
    };

    class Text : public SvgObject<Text> {
    public:
        Text &SetPoint(Point p) {
            x_y = p;
            return *this;
        }

        Text &SetOffset(Point p) {
            dx_dy = p;
            return *this;
        }

        Text &SetFontSize(uint32_t sz) {
            font_size = sz;
            return *this;
        }

        Text &SetFontFamily(const string &font) {
            font_family = font;
            return *this;
        }

        Text &SetData(const string &data) {
            text = data;
            return *this;
        }

        explicit operator string() const override {
            stringstream ss;
            ss << "<text ";
            ss << get_base_params_xml();
            ss << "x=\"" << x_y.x << "\" y=\"" << x_y.y << "\" ";
            ss << "dx=\"" << dx_dy.x << "\" dy=\"" << dx_dy.y << "\" ";
            ss << "font-size=\"" << font_size << "\" ";
            if (font_family) {
                ss << "font-family=\"" << *font_family << "\" ";
            }
            ss << ">";
            ss << text;
            ss << "</text>";
            return ss.str();
        }

    private:
        Point x_y{0.0, 0.0}, dx_dy{0.0, 0.0};
        uint32_t font_size = 1;
        optional<string> font_family = nullopt;
        string text = "";
    };


    class Document {
    public:
        template<typename SvgObject_T>
        void Add(SvgObject_T svg_object) {
            svg_objects.emplace_back(move(svg_object));
        }

        void Render(ostream &out) {
            out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << endl;
            out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">" << endl;
            for (const auto &node : svg_objects) {  // TODO: const unique_ptr<SvgObject> &node
                out << visit([](const auto &node) {
                                 return string(node);
                             },
                             node) << endl;
            }
            out << "</svg>" << endl;
        }


    private:
        vector<variant<Circle, Polyline, Text>> svg_objects;

    };
}


int main() {
    Svg::Document svg;

    svg.Add(
            Svg::Polyline{}
                    .SetStrokeColor(Svg::Rgb{140, 198, 63})  // soft green
                    .SetStrokeWidth(16)
                    .SetStrokeLineCap("round")
                    .AddPoint({50, 50})
                    .AddPoint({250, 250})
    );

    for (const auto point : {Svg::Point{50, 50}, Svg::Point{250, 250}}) {
        svg.Add(
                Svg::Circle{}
                        .SetFillColor("white")
                        .SetRadius(6)
                        .SetCenter(point)
        );
    }

    svg.Add(
            Svg::Text{}
                    .SetPoint({50, 50})
                    .SetOffset({10, -10})
                    .SetFontSize(20)
                    .SetFontFamily("Verdana")
                    .SetFillColor("black")
                    .SetData("C")
    );
    svg.Add(
            Svg::Text{}
                    .SetPoint({250, 250})
                    .SetOffset({10, -10})
                    .SetFontSize(20)
                    .SetFontFamily("Verdana")
                    .SetFillColor("black")
                    .SetData("C++")
    );

    svg.Render(std::cout);

}
