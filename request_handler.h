#pragma once

#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

#include <deque>
#include <optional>
#include <set>
#include <string_view>

using Container_stops_points = std::deque<std::pair<svg::Point, std::string_view>>;

class RequestHandler {
public:
    RequestHandler(const tc::TransportCatalogue& transport_catalogue, const MapRenderer& renderer, const graph::Router<double>& router, const Transport_router& transport_router);

    const Bus_Route_Stat GetBusStat(const std::string_view bus_name) const;
    const BusesToStop GetBusesByStop(const std::string_view stop_name) const;
    svg::Document RenderMap() const;

    const std::optional<Route_Stat> GetRoute(const std::string_view from, const std::string_view to) const;

private:
    int GetUniqueStopsCount(const Bus* bus) const;
    double CalculateGPSLength(const Bus* bus) const;
    int CalculateRealLength(const Bus* bus) const;

private:
    const tc::TransportCatalogue& transport_catalogue_;
    const MapRenderer& renderer_;
    const graph::Router<double>& router_;
    const Transport_router& transport_router_;

private:
    std::deque<const Bus*> sorted_buses_;
    std::set<std::string_view> sorted_unique_stopnames_;
};

