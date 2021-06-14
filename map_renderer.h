#pragma once

#include "domain.h"
#include "geo.h"
#include "svg.h"

#include <string_view>
#include <vector>

struct RenderSettings {
    double width = 0;   // области карты в пикселях
    double height = 0;  // области карты в пикселях
    double padding = 0; // Вещественное число не меньше 0 и меньше min(width, height)/2

    double line_width = 0; // — толщина линий, которыми рисуются автобусные маршруты. 0 до 100000.

    double stop_radius = 1.0;                               // — радиус окружностей, которыми обозначаются остановки.  от 0 до 100000.
    int bus_label_font_size = 1;                            // — размер шрифта текста, которым отображаются названия автобусных маршрутов. от 0 до 100000.
    std::array<double, 2> bus_label_offset = { 0.0, 0.0 };  // — массив из двух элементов типа double, задающий смещение надписи с названием автобусного маршрута относительно координат конечной остановки на карте. Задаёт значения свойств dx и dy SVG-элемента <text>. Координаты смещения задаются в диапазоне от -100000 до 100000.
    int stop_label_font_size = 1;                           // — целое число, задающее размер шрифта текста, которым отображаются названия остановок. Целое число в диапазоне от 0 до 100000.
    std::array<double, 2> stop_label_offset = { 0.0, 0.0 }; // — массив из двух элементов типа double, задающий смещение надписи с названием остановки относительно координат остановки на карте. Задаёт значения свойств dx и dy SVG-элемента <text>. Координаты смещения задаются в диапазоне от -100000 до 100000.

    svg::Color underlayer_color; // — цвет подложки, отображаемой под названиями остановок и автобусных маршрутов. Формат хранения цвета будет описан ниже.

    double underlayer_width = 0.0; // — толщина подложки под названиями остановок и автобусных маршрутов. Задаёт значение атрибута stroke-width элемента <text>. Вещественное число в диапазоне от 0 до 100000.

    std::vector<svg::Color> color_palette; // — непустой массив, элементы которого задают цветовую палитру, используемую для визуализации маршрутов. Формат хранения цвета описан ниже.
};

using Container_stops_points = std::deque<std::pair<svg::Point, std::string_view>>;

class MapRenderer {
public:
    MapRenderer(const RenderSettings& rs)
        : rs_(rs) {
    }

    double GetHeight() const;
    double GetWidth() const;
    double GetPadding() const;
    size_t GetColorPaletteSize() const;

    void RenderBusPolyline(svg::Document& doc, const std::deque<svg::Point>& stops_points, size_t color_idx) const;
    void RenderBusName(svg::Document& doc, const svg::Point& pos, const std::string& name, size_t color_idx) const;

    void RenderBusStopsCycle(svg::Document& doc, const svg::Point& pos) const;
    void RenderBusStopsCycle(svg::Document& doc, const Container_stops_points& points) const;

    void RenderStopName(svg::Document& doc, const svg::Point& pos, std::string_view name) const;
    void RenderStopName(svg::Document& doc, const Container_stops_points& points) const;

private:
    const RenderSettings rs_;
};

// ---GPS to Point----
inline constexpr double EPSILON = 1e-6;

bool IsZero(double value);

class SphereProjector {
public:
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end, double max_width,
                    double max_height, double padding)
        : padding_(padding) {
        if (points_begin == points_end) {
            return;
        }

        const auto [left_it, right_it]
            = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
                  return lhs.lng < rhs.lng;
              });

        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        const auto [bottom_it, top_it]
            = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
                  return lhs.lat < rhs.lat;
              });

        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            zoom_coeff_ = *height_zoom;
        }
    }

    svg::Point operator()(geo::Coordinates coords) const {
        return { (coords.lng - min_lon_) * zoom_coeff_ + padding_,
                 (max_lat_ - coords.lat) * zoom_coeff_ + padding_ };
    }

private:
    double padding_ = 0;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};