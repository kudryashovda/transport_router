#pragma once

#include <array>
#include <deque>
#include <set>
#include <string>
#include <unordered_map>

struct Stop {
    std::string name;
    double latitude{};
    double longitude{};
    std::deque<std::pair<int, std::string>> distances_to_stops;
};

struct Bus {
    std::string name;
    std::deque<std::string> stops;
    bool is_roundtrip = false;
};

enum class RequestType {
    BUS,
    STOP,
    MAP,
    ROUTE
};  

struct Stat {
    int id{};
    RequestType type;
    std::unordered_map<std::string, std::string> key_values;
};

struct Bus_Route_Stat {
    std::string bus_name;
    int stops_count{};
    int unique_stops{};
    double length{};
    double curvature{};
};

struct BusesToStop {
    std::string stop_name;
    bool notFound = true;
    std::set<std::string_view> buses;
};
