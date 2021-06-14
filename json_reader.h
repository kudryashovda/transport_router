#pragma once

#include "json.h"
#include "json_builder.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "transport_router.h"

struct Parsed_Inputs_Queries {
    std::deque<Stop> stops;
    std::deque<Bus> buses;
    std::deque<Stat> queries;

    RenderSettings render_settings;
    Routing_settings routing_settings;
};

json::Document LoadJSON(std::istream& input);

Parsed_Inputs_Queries ParseJson(const json::Document& document);

svg::Color getColorFromJsonNode(const json::Node& node);

json::Node Generate_Error_Message_Dict(int id, std::string_view text);

json::Node Generate_TransportMap_Dict(int id, std::string_view raw_map_data);
json::Node GetTransportMapNode(const Stat& stat, const RequestHandler& rh);

json::Node Generate_Buses_List_Dict(int id, const std::set<std::string_view>& buses_list);
json::Node GetBusesListNode(const Stat& stat, const RequestHandler& rh);

json::Node Generate_Route_Stat_Dict(int id, const Bus_Route_Stat& bus_info);
json::Node GetBusInfoNode(const Stat& stat, const RequestHandler& rh);

json::Node Generate_Route_Dict(int id, const Route_Stat& route_stat);
json::Node GetRouteNode(const Stat& stat, const RequestHandler& rh);