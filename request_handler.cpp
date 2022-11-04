#include "request_handler.h"

#include <cmath>

RequestHandler::RequestHandler(const tc::TransportCatalogue &transport_catalogue, const MapRenderer &renderer,
                               const graph::Router<double> &router, const Transport_router &transport_router)
    : transport_catalogue_(transport_catalogue)
    , renderer_(renderer)
    , router_(router)
    , transport_router_(transport_router) {

    for (const Stop& stop : transport_catalogue_.GetAllStops()) {
        sorted_unique_stopnames_.insert(stop.name);
    }

    // Don't use std::set - buses may be added more than one time
    for (const Bus& bus : transport_catalogue_.GetAllBuses()) {
        sorted_buses_.push_back(&bus);
    }

    std::sort(sorted_buses_.begin(), sorted_buses_.end(), [](const Bus* lhs, const Bus* rhs) {
        return lhs->name < rhs->name;
    });
}

Bus_Route_Stat RequestHandler::GetBusStat(const std::string_view bus_name) const {
    Bus_Route_Stat bus_route;

    bus_route.bus_name = bus_name;

    const Bus* bus = transport_catalogue_.GetRouteByBusName(bus_name);
    if (bus == nullptr) {
        return bus_route;
    }

    bus_route.stops_count = static_cast<int>(bus->stops.size());

    if (!bus->is_roundtrip) {
        bus_route.stops_count = 2 * bus_route.stops_count - 1;
    }

    bus_route.unique_stops = GetUniqueStopsCount(bus);

    bus_route.length = CalculateRealLength(bus);

    double gps_length = CalculateGPSLength(bus);

    constexpr double EPSILON = 1e-6;

    if (std::abs(gps_length) < EPSILON) {
        bus_route.curvature = 0.0;
    } else {
        bus_route.curvature = bus_route.length / gps_length;
    }

    if (std::isnan(bus_route.curvature)) {
        bus_route.curvature = 0.0;
    }

    return bus_route;
}

BusesToStop RequestHandler::GetBusesByStop(const std::string_view stop_name) const {
    BusesToStop b2s;

    b2s.stop_name = stop_name;

    const Stop* stop = transport_catalogue_.GetStopByName(stop_name);
    if (stop == nullptr) {
        b2s.notFound = true;
        return b2s;
    } else {
        b2s.notFound = false;
    }

    b2s.buses = transport_catalogue_.GetBusesToStop(stop);

    return b2s;
}

svg::Document RequestHandler::RenderMap() const {
    svg::Document doc;

    // Get all gps points to calculate canvas size exclude that without buses
    std::deque<geo::Coordinates> geo_points;

    for (std::string_view stop_name : sorted_unique_stopnames_) {
        if (transport_catalogue_.GetBusesToStop(transport_catalogue_.GetStopByName(stop_name)).empty()) {
            continue;
        }

        double lat = transport_catalogue_.GetStopByName(stop_name)->latitude;
        double lng = transport_catalogue_.GetStopByName(stop_name)->longitude;
        geo_points.push_back({ lat, lng });
    }

    // Calculate inner coeffs to convert geo coords to x,y points
    SphereProjector sp(geo_points.begin(), geo_points.end(), renderer_.GetWidth(), renderer_.GetHeight(), renderer_.GetPadding());

    // To color routes with repeated color
    const size_t colors_in_palete = renderer_.GetColorPaletteSize();

    size_t color_idx = 0;

    // draw polyline
    for (const Bus* bus_ptr : sorted_buses_) {

        std::deque<svg::Point> stops_points;

        for (std::string_view stop : bus_ptr->stops) {
            double lat = transport_catalogue_.GetStopByName(stop)->latitude;
            double lng = transport_catalogue_.GetStopByName(stop)->longitude;
            stops_points.push_back(sp({ lat, lng }));
        }

        // for line bus reverse route
        if (!bus_ptr->is_roundtrip) {
            auto it = bus_ptr->stops.rbegin() + 1;

            while (it != bus_ptr->stops.rend()) {
                double lat = transport_catalogue_.GetStopByName(*it)->latitude;
                double lng = transport_catalogue_.GetStopByName(*it)->longitude;
                stops_points.push_back(sp({ lat, lng }));
                ++it;
            }
        }

        renderer_.RenderBusPolyline(doc, stops_points, color_idx);

        ++color_idx;

        if (color_idx == colors_in_palete) {
            color_idx = 0;
        }
    }

    // draw bus names
    color_idx = 0;
    for (const Bus* bus_ptr : sorted_buses_) {

        // render first stop in all cases
        std::string_view first_stop = bus_ptr->stops.front();

        double lat = transport_catalogue_.GetStopByName(first_stop)->latitude;
        double lng = transport_catalogue_.GetStopByName(first_stop)->longitude;

        renderer_.RenderBusName(doc, sp({ lat, lng }), bus_ptr->name, color_idx);

        // Draw second text if bus is not round or line bus with same fin stops
        std::string_view last_stop = bus_ptr->stops.back();
        bool is_same_first_last_stops = (first_stop == last_stop);

        if (!bus_ptr->is_roundtrip && !is_same_first_last_stops) {
            lat = transport_catalogue_.GetStopByName(last_stop)->latitude;
            lng = transport_catalogue_.GetStopByName(last_stop)->longitude;

            renderer_.RenderBusName(doc, sp({ lat, lng }), bus_ptr->name, color_idx);
        }

        ++color_idx;

        if (color_idx == colors_in_palete) {
            color_idx = 0;
        }
    }

    // Add stops cycles excl stops without buses
    std::deque<std::pair<svg::Point, std::string_view>> stops_w_buses_points;

    for (std::string_view stop_name : sorted_unique_stopnames_) {
        if (transport_catalogue_.GetBusesToStop(transport_catalogue_.GetStopByName(stop_name)).empty()) {
            continue;
        }

        double lat = transport_catalogue_.GetStopByName(stop_name)->latitude;
        double lng = transport_catalogue_.GetStopByName(stop_name)->longitude;

        stops_w_buses_points.emplace_back(sp({ lat, lng }), stop_name);
    }

    renderer_.RenderBusStopsCycle(doc, stops_w_buses_points);

    renderer_.RenderStopName(doc, stops_w_buses_points);

    return doc;
}

// ----Auxiliary private methods----
double RequestHandler::CalculateGPSLength(const Bus* bus) const {

    const auto& stops = bus->stops;

    double direct_length = 0.0;
    for (auto it = stops.begin(); it + 1 != stops.end(); ++it) {
        auto* stop_prev = transport_catalogue_.GetStopByName(*it);
        auto* stop_next = transport_catalogue_.GetStopByName(*(std::next(it)));

        direct_length += geo::ComputeDistance({ stop_prev->latitude, stop_prev->longitude }, { stop_next->latitude, stop_next->longitude });
    }

    if (!bus->is_roundtrip) {
        return 2 * direct_length;
    }

    return direct_length;
}

int RequestHandler::CalculateRealLength(const Bus* bus) const {
    const auto& stops = bus->stops;

    int length = 0.0;
    for (auto it = stops.begin(); it + 1 != stops.end(); ++it) {
        auto* stop_prev = transport_catalogue_.GetStopByName(*it);
        auto* stop_next = transport_catalogue_.GetStopByName(*(std::next(it)));

        auto distance_prev_next = transport_catalogue_.GetDistanceByStopsPair(stop_prev, stop_next);
        auto distance_next_prev = transport_catalogue_.GetDistanceByStopsPair(stop_next, stop_prev);

        if (distance_prev_next == std::nullopt) {
            throw std::logic_error("Can't find stops distance"s);
        }

        if (bus->is_roundtrip) {
            length += distance_prev_next.value();
        } else {
            if (distance_next_prev == std::nullopt) {
                throw std::logic_error("Can't find stops distance"s);
            }

            length += distance_prev_next.value() + distance_next_prev.value();
        }
    }
    return length;
}

int RequestHandler::GetUniqueStopsCount(const Bus* bus) {
    std::unordered_set<std::string_view> unique_stops(bus->stops.begin(), bus->stops.end());

    return static_cast<int>(unique_stops.size());
}

std::optional<Route_Stat> RequestHandler::GetRoute(std::string_view from, std::string_view to) const {
    const Routing_settings routing_settings = transport_router_.GetRouterSettings();

    const Stop* stop_from = transport_catalogue_.GetStopByName(from);
    const Stop* stop_to = transport_catalogue_.GetStopByName(to);

    graph::VertexId idx_stop_from = transport_catalogue_.GetStopIndex(stop_from);
    graph::VertexId idx_stop_to = transport_catalogue_.GetStopIndex(stop_to);

    std::optional<graph::Router<double>::RouteInfo> route_info = router_.BuildRoute(idx_stop_from, idx_stop_to);

    if (route_info == std::nullopt) {
        return std::nullopt;
    }

    Route_Stat route_stat;

    const std::vector<graph::EdgeId> edges = route_info.value().edges;

    double total_time = 0.0;

    if (edges.empty()) {
        Route_Stat empty_route;
        empty_route.total_time = 0.0;

        return empty_route;
    }

    for (const auto& edgeID : edges) {
        Edge_props props = transport_router_.GetEdgeProps(edgeID);

        Route_Element wait_element;
        wait_element.stop_name = props.stop_from;
        wait_element.time = routing_settings.bus_wait_time;
        wait_element.type = "Wait"s;
        route_stat.items.push_back(std::move(wait_element));

        Route_Element go_element;
        go_element.time = props.travel_time - routing_settings.bus_wait_time;
        go_element.type = "Bus"s;
        go_element.bus_name = props.bus->name;
        go_element.span_count = props.span_count;
        route_stat.items.push_back(std::move(go_element));

        total_time += props.travel_time;
    }

    route_stat.total_time = total_time;

    route_stat.bus_wait_time = routing_settings.bus_wait_time;

    return route_stat;
}
