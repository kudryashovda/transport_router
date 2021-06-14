#include "transport_catalogue.h"

using namespace std;

namespace tc {

    void TransportCatalogue::AddStopToBase(const Stop& stop) {
        const auto& link = stops_.emplace_back(stop);
        names_to_stops_[link.name] = &link;

        stop_to_idx_[&link] = stop_to_idx_.size(); // to draw graph by vertexIDs
    }

    size_t TransportCatalogue::GetStopIndex(const Stop* stop) const {
        return stop_to_idx_.at(stop);
    }

    void TransportCatalogue::AddStopDistancesToBase(const Stop& stop) {
        const Stop* stop_from = GetStopByName(stop.name);

        for (const auto& distance_to_stop_name : stop.distances_to_stops) {
            const Stop* stop_to = GetStopByName(distance_to_stop_name.second);

            StopsDistance_to_length_.emplace(make_pair(stop_from, stop_to), distance_to_stop_name.first);
        }
    }

    void TransportCatalogue::AddRouteToBase(const Bus& bus) {
        const auto& link = buses_.emplace_back(bus);
        names_to_buses_[link.name] = &link;

        for (const auto& stop_name : link.stops) {
            const Stop* stop = GetStopByName(stop_name);
            stop_to_buses_[stop].insert(link.name);
        }
    }

    void TransportCatalogue::FillTransportBase(const std::deque<Stop>& stops, const std::deque<Bus>& buses) {
        // fill all stops to base
        for (auto& stop : stops) {
            AddStopToBase(stop);
        }

        // fill all distances
        for (auto& stop : stops) {
            AddStopDistancesToBase(stop);
        }

        // fill all routes
        for (auto& bus : buses) {
            AddRouteToBase(bus);
        }
    }

    const Bus* TransportCatalogue::GetRouteByBusName(std::string_view bus_name) const {
        auto it = names_to_buses_.find(bus_name);

        if (it != names_to_buses_.end()) {
            return it->second;
        } else {
            return nullptr;
        }
    }

    const Stop* TransportCatalogue::GetStopByName(std::string_view stop_name) const {
        auto it = names_to_stops_.find(stop_name);

        if (it != names_to_stops_.end()) {
            return it->second;
        } else {
            return nullptr;
        }
    }

    const set<string_view>& TransportCatalogue::GetBusesToStop(const Stop* stop) const {
        const static set<string_view> empty_set;

        auto it = stop_to_buses_.find(stop);

        if (it == stop_to_buses_.end()) {
            return empty_set;
        }

        return it->second;
    }

    std::optional<int> TransportCatalogue::GetDistanceByStopsPair(const Stop* stop_from, const Stop* stop_to) const {
        auto it = StopsDistance_to_length_.find(std::make_pair(stop_from, stop_to));

        if (it == StopsDistance_to_length_.end()) {
            auto it_reverse = StopsDistance_to_length_.find(std::make_pair(stop_to, stop_from));

            if (it_reverse == StopsDistance_to_length_.end()) {
                return std::nullopt;
            }

            return it_reverse->second;
        }

        return it->second;
    }

    const std::deque<Stop>& TransportCatalogue::GetAllStops() const {
        return stops_;
    }

    const std::deque<Bus>& TransportCatalogue::GetAllBuses() const {
        return buses_;
    }

    size_t TransportCatalogue::GetAllStopsCount() const {
        return stop_to_idx_.size();
    }

} // namespace tc