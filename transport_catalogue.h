#pragma once

#include "domain.h"
#include "geo.h"

#include <algorithm>
#include <deque>
#include <numeric>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace tc {

    struct StopsDistanceHash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2>& pair) const {
            return std::hash<T1>()(pair.first) + 37 * std::hash<T2>()(pair.second);
        }
    };

    class TransportCatalogue {
    public:
        void AddRouteToBase(const Bus& bus);
        void AddStopToBase(const Stop& stop);
        void AddStopDistancesToBase(const Stop& stop);
        void FillTransportBase(const std::deque<Stop>& stops, const std::deque<Bus>& buses);

        const Bus* GetRouteByBusName(std::string_view bus_name) const;
        const Stop* GetStopByName(std::string_view stop_name) const;
        const std::set<std::string_view>& GetBusesToStop(const Stop* stop) const;
        std::optional<int> GetDistanceByStopsPair(const Stop* stop_from, const Stop* stop_to) const;

        const std::deque<Stop>& GetAllStops() const;
        const std::deque<Bus>& GetAllBuses() const;

        size_t GetAllStopsCount() const;
        size_t GetStopIndex(const Stop* stop) const;

    private:
        std::unordered_map<const Stop*, size_t> stop_to_idx_;
        std::deque<Stop> stops_;
        std::deque<Bus> buses_;

        std::unordered_map<std::string_view, const Stop*> names_to_stops_;
        std::unordered_map<std::string_view, const Bus*> names_to_buses_;

        std::unordered_map<const Stop*, std::set<std::string_view>> stop_to_buses_;
        std::unordered_map<std::pair<const Stop*, const Stop*>, int, StopsDistanceHash> StopsDistance_to_length_;
    };
} // namespace tc