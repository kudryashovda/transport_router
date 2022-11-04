#pragma once

#include "router.h"
#include "transport_catalogue.h"

struct Routing_settings {
    int bus_wait_time{};
    double bus_velocity{};
};

struct Route_Element {
    std::string type;
    std::string stop_name;
    std::string bus_name;
    double time{};
    int span_count{};
};

struct Route_Stat {
    int id{};
    double total_time{};
    std::deque<Route_Element> items;
    double bus_wait_time{};
};

struct Edge_props {
    const Bus* bus;
    int span_count{};
    int distance{};
    double travel_time{};
    std::string_view stop_from;
};

class Transport_router {

public:
    Transport_router(graph::DirectedWeightedGraph<double> &routes_graph,
                     const tc::TransportCatalogue &transport_catalogue, const Routing_settings &routing_settings)
        : routes_graph_(routes_graph)
        , transport_catalogue_(transport_catalogue)
        , routing_settings_(routing_settings) {
    }

    void CreateGraph();

    const Edge_props& GetEdgeProps(graph::EdgeId) const;
    const Routing_settings& GetRouterSettings() const;

private:
    graph::DirectedWeightedGraph<double>& routes_graph_; // will be modified
    const tc::TransportCatalogue& transport_catalogue_;
    const Routing_settings& routing_settings_;

    std::unordered_map<graph::EdgeId, Edge_props> edgeID_to_edge_props_;
    std::unordered_map<std::pair<graph::VertexId, graph::VertexId>, int, tc::StopsDistanceHash> pair_idx_to_distance_;
};
