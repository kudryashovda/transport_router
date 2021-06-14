#pragma once

#include <algorithm>
#include <cstdint>
#include <deque>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <variant>

using namespace std::literals;

namespace svg {

    struct Point {
        Point() = default;

        Point(double x, double y)
            : x(x)
            , y(y) {
        }

        double x = 0;
        double y = 0;
    };

    struct RenderContext {
        RenderContext(std::ostream& out)
            : out(out) {
        }

        RenderContext(std::ostream& out, int indent_step, int indent = 0)
            : out(out)
            , indent_step(indent_step)
            , indent(indent) {
        }

        RenderContext Indented() const {
            return { out, indent_step, indent + indent_step };
        }

        void RenderIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        std::ostream& out;
        int indent_step = 0;
        int indent = 0;
    };

    class Object {
    public:
        void Render(const RenderContext& context) const;

        virtual ~Object() = default;

    private:
        virtual void RenderObject(const RenderContext& context) const = 0;
    };

    // -------PathProps--------
    struct Rgb {
        Rgb(){};
        Rgb(uint8_t red, uint8_t green, uint8_t blue)
            : red(red)
            , green(green)
            , blue(blue) {
        }
        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
    };

    struct Rgba {
        Rgba(){};
        Rgba(uint8_t red, uint8_t green, uint8_t blue, double opacity)
            : red(red)
            , green(green)
            , blue(blue)
            , opacity(opacity) {
        }
        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
        double opacity = 1.0;
    };

    using Color = std::variant<std::monostate, std::string, svg::Rgb, svg::Rgba>;

    inline const Color NoneColor{ "none" };

    // Visitor for Color
    struct ColorPrinter {
        std::ostream& out;

        void operator()(std::monostate) const {
            out << "none"sv;
        }
        void operator()(std::string_view str) const {
            out << str;
        }
        void operator()(svg::Rgb rgb) const {
            out << "rgb("sv << static_cast<int>(rgb.red) << ","sv << static_cast<int>(rgb.green) << ","sv << static_cast<int>(rgb.blue) << ")"sv;
        }
        void operator()(svg::Rgba rgba) const {
            out << "rgba("sv << static_cast<int>(rgba.red) << ","sv << static_cast<int>(rgba.green) << ","sv << static_cast<int>(rgba.blue) << ","sv << rgba.opacity << ")"sv;
        }
    };

    enum class StrokeLineCap {
        BUTT,
        ROUND,
        SQUARE,
    };

    std::ostream& operator<<(std::ostream& os, svg::StrokeLineCap slc);

    enum class StrokeLineJoin {
        ARCS,
        BEVEL,
        MITER,
        MITER_CLIP,
        ROUND,
    };

    std::ostream& operator<<(std::ostream& os, StrokeLineJoin slj);

    template <typename Owner>
    class PathProps {
    public:
        Owner& SetFillColor(Color color) {
            fill_color_ = std::move(color);
            return AsOwner();
        }
        Owner& SetStrokeColor(Color color) {
            stroke_color_ = std::move(color);
            return AsOwner();
        }

        Owner& SetStrokeWidth(double width) {
            strokeWidth_ = width;
            return AsOwner();
        }

        Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
            strokeLineCap_ = line_cap;
            return AsOwner();
        }

        Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
            strokeLineJoin_ = line_join;
            return AsOwner();
        }

    protected:
        ~PathProps() = default;

        void RenderAttrs(std::ostream& out) const {
            using namespace std::literals;

            std::ostringstream strm;

            if (fill_color_) {
                std::visit(ColorPrinter{ strm }, fill_color_.value());
                out << " fill=\""sv << strm.str() << "\""sv;
            }

            std::ostringstream strm2;
            if (stroke_color_) {
                std::visit(ColorPrinter{ strm2 }, stroke_color_.value());
                out << " stroke=\""sv << strm2.str() << "\""sv;
            }

            if (strokeWidth_) {
                out << " stroke-width=\""sv << *strokeWidth_ << "\""sv;
            }

            if (strokeLineCap_) {
                out << " stroke-linecap=\""sv << *strokeLineCap_ << "\""sv;
            }

            if (strokeLineJoin_) {
                out << " stroke-linejoin=\""sv << *strokeLineJoin_ << "\""sv;
            }
        }

    private:
        Owner& AsOwner() {
            return static_cast<Owner&>(*this);
        }

        std::optional<Color> fill_color_;
        std::optional<Color> stroke_color_;
        std::optional<double> strokeWidth_;
        std::optional<StrokeLineCap> strokeLineCap_;
        std::optional<StrokeLineJoin> strokeLineJoin_;
    };
    // --- end of PathProps ----

    class Circle final : public Object, public PathProps<Circle> {
    public:
        Circle& SetCenter(Point center);
        Circle& SetRadius(double radius);

    private:
        void RenderObject(const RenderContext& context) const override;

        Point center_;
        double radius_ = 1.0;
    };

    class Polyline final : public Object, public PathProps<Polyline> {
    public:
        Polyline& AddPoint(Point point);

    private:
        void RenderObject(const RenderContext& context) const override;

        std::deque<Point> points_;
    };

    class Text final : public Object, public PathProps<Text> {
    public:
        Text& SetPosition(Point pos);
        Text& SetOffset(Point offset);
        Text& SetFontSize(uint32_t size);
        Text& SetFontFamily(std::string font_family);
        Text& SetFontWeight(std::string font_weight);
        Text& SetData(std::string data);

    private:
        void RenderObject(const RenderContext& context) const override;
        std::string RemoveOutSpaces(const std::string& str) const;
        std::string SpecialSymbolsShield(const std::string& text) const;

        Point pos_;
        Point offset_;
        uint32_t size_ = 1;
        std::string font_family_;
        std::string font_weight_;
        std::string data_;
    };

    class ObjectContainer {
    public:
        template <typename Object>
        void Add(Object obj) {
            objects_.emplace_back(std::make_unique<Object>(std::move(obj)));
        }

        virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;

        virtual ~ObjectContainer() = default;

    protected:
        std::deque<std::shared_ptr<Object>> objects_;
    };

    class Drawable {
    public:
        virtual void Draw(ObjectContainer& oc) const = 0;

        virtual ~Drawable() = default;
    };

    class Document : public ObjectContainer {
    public:
        void AddPtr(std::unique_ptr<Object>&& obj) override;
        void Render(std::ostream& out) const;
    };

} // namespace svg