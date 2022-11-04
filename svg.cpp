#include "svg.h"

namespace svg {

    using namespace std::literals;

    std::ostream& operator<<(std::ostream& os, svg::StrokeLineCap slc) {
        switch (slc) {
        case svg::StrokeLineCap::BUTT:
            os << "butt"sv;
            break;
        case svg::StrokeLineCap::ROUND:
            os << "round"sv;
            break;
        case svg::StrokeLineCap::SQUARE:
            os << "square"sv;
            break;
        default:
            break;
        }

        return os;
    }

    std::ostream& operator<<(std::ostream& os, StrokeLineJoin slj) {
        switch (slj) {
        case StrokeLineJoin::ARCS:
            os << "arcs"sv;
            break;
        case StrokeLineJoin::BEVEL:
            os << "bevel"sv;
            break;
        case StrokeLineJoin::MITER:
            os << "miter"sv;
            break;
        case StrokeLineJoin::MITER_CLIP:
            os << "miter-clip"sv;
            break;
        case StrokeLineJoin::ROUND:
            os << "round"sv;
            break;
        default:
            break;
        }

        return os;
    }

    // ---------- Document ------------------
    void Document::AddPtr(std::unique_ptr<Object>&& obj) {
        objects_.emplace_back(std::move(obj));
    }

    void Document::Render(std::ostream& out) const {
        constexpr int indent_step = 2;
        constexpr int indent = 2;

        RenderContext ctx(out, indent_step, indent);

        const std::string_view xml_header = R"(<?xml version="1.0" encoding="UTF-8" ?>)";
        const std::string_view svg_header = R"(<svg xmlns="http://www.w3.org/2000/svg" version="1.1">)";

        out << xml_header << "\n"sv;
        out << svg_header << "\n"sv;

        for (const auto& it : objects_) {
            it->Render(ctx);
        }

        out << "</svg>"sv;
    }

    // ---------- Render ------------------
    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        RenderObject(context);

        context.out << "\n"sv;
    }

    // ---------- Circle ------------------
    Circle& Circle::SetCenter(Point center) {
        center_ = center;

        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;

        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;

        out << "<circle cx=\""sv;
        out << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\" "sv;
        RenderAttrs(context.out);
        out << "/>"sv;
    }

    // ---------- Poyline ------------------
    Polyline& Polyline::AddPoint(Point point) {
        points_.push_back(point);

        return *this;
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        auto& out = context.out;

        out << "<polyline points=\""sv;

        for (size_t i = 0; i < points_.size(); ++i) {
            out << points_[i].x << ","sv << points_[i].y;

            if (i + 1 != points_.size()) {
                out << " "sv;
            }
        }

        out << "\""sv;
        RenderAttrs(context.out);
        out << " />"sv;
    }

    // ---------- Text ------------------
    Text& Text::SetPosition(Point pos) {
        pos_ = pos;

        return *this;
    }

    Text& Text::SetOffset(Point offset) {
        offset_ = offset;

        return *this;
    }

    Text& Text::SetFontSize(uint32_t size) {
        size_ = size;

        return *this;
    }

    Text& Text::SetFontFamily(std::string font_family) {
        font_family_ = std::move(font_family);

        return *this;
    }

    Text& Text::SetFontWeight(std::string font_weight) {
        font_weight_ = std::move(font_weight);

        return *this;
    }

    Text& Text::SetData(std::string data) {
        data_ = std::move(data);

        return *this;
    }

    std::string Text::RemoveOutSpaces(const std::string& str) {
        auto left_nspace_pos = str.find_first_not_of(' ');
        auto right_nspace_pos = str.find_last_not_of(' ');

        // if string has only spaces - remove them
        if (left_nspace_pos == std::string::npos && right_nspace_pos == std::string::npos) {
            return {};
        }

        return str.substr(left_nspace_pos, right_nspace_pos - left_nspace_pos + 1);
    }

    std::string Text::SpecialSymbolsShield(const std::string& str) {
        std::string out_str;

        for (char current_char : str) {
            if (current_char == '"') {
                out_str += "&quot;"sv;
                continue;
            }

            if (current_char == '\'' || current_char == '`') {
                out_str += "&apos;"sv;
                continue;
            }

            if (current_char == '<') {
                out_str += "&lt;"sv;
                continue;
            }

            if (current_char == '>') {
                out_str += "&gt;"sv;
                continue;
            }

            if (current_char == '&') {
                out_str += "&amp;"sv;
                continue;
            }

            out_str += current_char;
        }

        return out_str;
    }

    void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;

        out << "<text x=\""sv << pos_.x << "\" y=\""sv << pos_.y << "\" "sv
            << "dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\" "sv
            << "font-size=\""sv << size_ << "\""sv;

        if (!font_family_.empty()) {
            out << " font-family=\""sv << font_family_ << "\""sv;
        }

        if (!font_weight_.empty()) {
            out << " font-weight=\""sv << font_weight_ << "\""sv;
        }

        RenderAttrs(context.out);

        out << ">"sv;

        out << RemoveOutSpaces(SpecialSymbolsShield(data_));

        out << "</text>"sv;
    }

} // namespace svg