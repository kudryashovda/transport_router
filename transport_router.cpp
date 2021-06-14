#include "transport_router.h"

using namespace std;

void Transport_router::CreateGraph() {
    // dictionary for put and after remove biggest edges and trasfer later to graph.AddEdge
    std::unordered_map<std::pair<graph::VertexId, graph::VertexId>, Edge_props, tc::StopsDistanceHash> tmp_pair_idx_to_distance;

    graph::VertexId idx_stop_from = 0;

    graph::VertexId idx_prev_stop_to = 0;
    graph::VertexId idx_current_stop_to = 0;

    for (const Bus& bus : transport_catalogue_.GetAllBuses()) {

        for (auto it_outer = bus.stops.begin(); it_outer + 1 != bus.stops.end(); ++it_outer) {
            int span_count = 0;

            const Stop* stop_from = transport_catalogue_.GetStopByName(*it_outer);
            idx_stop_from = transport_catalogue_.GetStopIndex(stop_from);

            constexpr int zero_stop_distance = 0;
            // it is a temporary storage for subsequent distance/time calculation
            pair_idx_to_distance_[{ idx_stop_from, idx_stop_from }] = zero_stop_distance;

            for (auto it_inner = it_outer + 1; it_inner != bus.stops.end(); ++it_inner) {
                ++span_count;

                const Stop* prev_stop = transport_catalogue_.GetStopByName(*std::prev(it_inner));
                const Stop* current_stop = transport_catalogue_.GetStopByName(*it_inner);

                idx_prev_stop_to = transport_catalogue_.GetStopIndex(prev_stop);
                idx_current_stop_to = transport_catalogue_.GetStopIndex(current_stop);

                auto distance_prev_current = transport_catalogue_.GetDistanceByStopsPair(prev_stop, current_stop);
                if (distance_prev_current == std::nullopt) {
                    throw std::logic_error("Can't find stops distance"s);
                }

                int distance_from_to_current = pair_idx_to_distance_.at({ idx_stop_from, idx_prev_stop_to })
                                               + distance_prev_current.value();
                pair_idx_to_distance_[{ idx_stop_from, idx_current_stop_to }] = distance_from_to_current;

                // to out index
                Edge_props edge_prop;

                edge_prop.bus = &bus;
                edge_prop.span_count = span_count;
                edge_prop.distance = distance_from_to_current;
                edge_prop.travel_time = 0.0; // calculate later
                edge_prop.stop_from = *it_outer;

                auto it_find = tmp_pair_idx_to_distance.find({ idx_stop_from, idx_current_stop_to });

                if (it_find == tmp_pair_idx_to_distance.end()) {
                    tmp_pair_idx_to_distance[{ idx_stop_from, idx_current_stop_to }] = edge_prop;
                } else if (it_find->second.distance > distance_from_to_current) {
                    tmp_pair_idx_to_distance[{ idx_stop_from, idx_current_stop_to }] = edge_prop;
                }

                if (!bus.is_roundtrip) {
                    auto distance_current_prev = transport_catalogue_.GetDistanceByStopsPair(current_stop, prev_stop);
                    if (distance_current_prev == std::nullopt) {
                        throw std::logic_error("can't find stops distance"s);
                    }

                    int distance_current_to_from = pair_idx_to_distance_.at({ idx_prev_stop_to, idx_stop_from })
                                                   + distance_current_prev.value();
                    pair_idx_to_distance_[{ idx_current_stop_to, idx_stop_from }] = distance_current_to_from;

                    // change some fields in edge_prop struct
                    Edge_props edge_prop_rev(edge_prop);

                    edge_prop_rev.distance = distance_current_to_from;
                    edge_prop_rev.stop_from = *it_inner;

                    // to out index
                    auto it_find_rev = tmp_pair_idx_to_distance.find({ idx_current_stop_to, idx_stop_from });

                    if (it_find_rev == tmp_pair_idx_to_distance.end()) {
                        tmp_pair_idx_to_distance[{ idx_current_stop_to, idx_stop_from }] = edge_prop_rev;
                    } else if (it_find_rev->second.distance > distance_current_to_from) {
                        tmp_pair_idx_to_distance[{ idx_current_stop_to, idx_stop_from }] = edge_prop_rev;
                    }
                }
            }
        }

        for (const auto& [stops_indexes, props] : tmp_pair_idx_to_distance) {

            double travel_time = double(props.distance) / routing_settings_.bus_velocity + double(routing_settings_.bus_wait_time);

            graph::EdgeId id = routes_graph_.AddEdge({ stops_indexes.first, stops_indexes.second, travel_time });

            Edge_props edge_prop(props);
            edge_prop.travel_time = travel_time;

            edgeID_to_edge_props_.emplace(id, std::move(edge_prop));
        }
    }
}

const Edge_props& Transport_router::GetEdgeProps(graph::EdgeId id) const {
    return edgeID_to_edge_props_.at(id);
}

const Routing_settings& Transport_router::GetRouterSettings() const {
    return routing_settings_;
}