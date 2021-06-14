#include "json_reader.h"

using namespace std::literals;

json::Document LoadJSON(std::istream& input) {
    return json::Load(input);
}

json::Node Generate_Error_Message_Dict(int id, std::string_view text) {

    json::Node result = json::Builder()
                            .StartDict()
                            .Key("request_id"s)
                            .Value(id)
                            .Key("error_message"s)
                            .Value(std::string(text))
                            .EndDict()
                            .Build();

    return result;
}

json::Node GetTransportMapNode(const Stat& stat, const RequestHandler& rh) {
    svg::Document doc_map = rh.RenderMap();

    std::stringstream ss;
    doc_map.Render(ss);

    return Generate_TransportMap_Dict(stat.id, ss.str());
}

json::Node Generate_TransportMap_Dict(int id, std::string_view raw_map_data) {
    json::Node result = json::Builder()
                            .StartDict()
                            .Key("request_id"s)
                            .Value(id)
                            .Key("map"s)
                            .Value(std::string(raw_map_data))
                            .EndDict()
                            .Build();

    return result;
}

json::Node GetBusesListNode(const Stat& stat, const RequestHandler& rh) {
    const BusesToStop& stop_info = rh.GetBusesByStop(stat.key_values.at("name"s));

    if (stop_info.notFound) {
        return Generate_Error_Message_Dict(stat.id, "not found"sv);
    } else {
        return Generate_Buses_List_Dict(stat.id, stop_info.buses);
    }
}

json::Node Generate_Buses_List_Dict(int id, const std::set<std::string_view>& buses_list) {
    json::Array j_buses;

    for (const auto& bus : buses_list) {
        j_buses.push_back(std::string(bus));
    }

    json::Node result = json::Builder()
                            .StartDict()
                            .Key("buses"s)
                            .Value(std::move(j_buses))
                            .Key("request_id"s)
                            .Value(id)
                            .EndDict()
                            .Build();

    return result;
}

json::Node GetBusInfoNode(const Stat& stat, const RequestHandler& rh) {
    const Bus_Route_Stat& bus_info = rh.GetBusStat(stat.key_values.at("name"s));

    if (bus_info.stops_count == 0) {
        return Generate_Error_Message_Dict(stat.id, "not found"sv);
    } else {
        return Generate_Route_Stat_Dict(stat.id, bus_info);
    }
}

json::Node Generate_Route_Stat_Dict(int id, const Bus_Route_Stat& bus_route) {
    json::Node result = json::Builder()
                            .StartDict()
                            .Key("curvature"s)
                            .Value(bus_route.curvature)
                            .Key("request_id"s)
                            .Value(id)
                            .Key("route_length"s)
                            .Value(bus_route.length)
                            .Key("stop_count"s)
                            .Value(bus_route.stops_count)
                            .Key("unique_stop_count"s)
                            .Value(bus_route.unique_stops)
                            .EndDict()
                            .Build();

    return result;
}

json::Node GetRouteNode(const Stat& stat, const RequestHandler& rh) {

    std::string from = stat.key_values.at("from");
    std::string to = stat.key_values.at("to");

    std::optional<Route_Stat> route_stat_opt = rh.GetRoute(from, to);

    if (route_stat_opt == std::nullopt) {
        return Generate_Error_Message_Dict(stat.id, "not found"sv);
    } else {
        return Generate_Route_Dict(stat.id, route_stat_opt.value());
    }
}

json::Node Generate_Route_Dict(int id, const Route_Stat& route_stat) {

    json::Array j_array;

    for (const auto& item : route_stat.items) {
        if (item.type == "Wait"s) {
            json::Dict dict;

            dict.emplace("type"s, "Wait"s);
            dict.emplace("stop_name"s, item.stop_name);
            dict.emplace("time"s, route_stat.bus_wait_time);

            j_array.push_back(std::move(dict));
        }

        if (item.type == "Bus"s) {
            json::Dict dict;

            dict.emplace("type"s, "Bus"s);
            dict.emplace("bus"s, item.bus_name);
            dict.emplace("span_count"s, item.span_count);
            dict.emplace("time"s, item.time);

            j_array.push_back(std::move(dict));
        }
    }

    json::Node result = json::Builder()
                            .StartDict()
                            .Key("request_id"s)
                            .Value(id)
                            .Key("total_time"s)
                            .Value(route_stat.total_time)
                            .Key("items"s)
                            .Value(std::move(j_array))
                            .EndDict()
                            .Build();

    return result;
}

Parsed_Inputs_Queries ParseJson(const json::Document& document) {

    Parsed_Inputs_Queries parsed;

    const auto root_dict = document.GetRoot().AsDict();

    const auto& bases_stops_dict_it = root_dict.find("base_requests"s);
    const auto& render_settings_dict_it = root_dict.find("render_settings"s);
    const auto& stats_dict_it = root_dict.find("stat_requests"s);
    const auto& routing_settings_dict_it = root_dict.find("routing_settings"s);

    // Read bus-stop array from json
    if (bases_stops_dict_it != root_dict.end()) {

        for (const auto& it : bases_stops_dict_it->second.AsArray()) {

            const auto& entry_dict = it.AsDict();

            const auto query_type_it = entry_dict.find("type"s);

            // bus or stop
            if (query_type_it != entry_dict.end()) {

                const auto& type_name = query_type_it->second.AsString();

                if (type_name == "Bus"s) {

                    Bus bus;
                    bus.name = entry_dict.at("name"s).AsString();

                    for (auto stop : entry_dict.at("stops"s).AsArray()) {
                        bus.stops.push_back(std::move(stop.AsString()));
                    }

                    bus.is_roundtrip = entry_dict.at("is_roundtrip"s).AsBool();

                    parsed.buses.push_back(std::move(bus));
                }

                if (type_name == "Stop"s) {

                    Stop stop;
                    stop.name = entry_dict.at("name"s).AsString();
                    stop.latitude = entry_dict.at("latitude"s).AsDouble();
                    stop.longitude = entry_dict.at("longitude"s).AsDouble();

                    for (const auto& [stop_name, distance] : entry_dict.at("road_distances"s).AsDict()) {
                        stop.distances_to_stops.emplace_back(distance.AsInt(), stop_name);
                    }

                    parsed.stops.push_back(std::move(stop));
                }
            }
        }
    }

    if (routing_settings_dict_it != root_dict.end()) {
        const auto routing_map = routing_settings_dict_it->second.AsDict();

        parsed.routing_settings.bus_wait_time = routing_map.at("bus_wait_time"s).AsInt();

        // convert velocity in km/h coefficient to calculate time in minutes
        constexpr int meters_in_km = 1000;
        constexpr int minutes_in_hour = 60;
        parsed.routing_settings.bus_velocity = meters_in_km / double(minutes_in_hour) * routing_map.at("bus_velocity"s).AsDouble();
    }

    if (render_settings_dict_it != root_dict.end()) {
        const auto render_map = render_settings_dict_it->second.AsDict();

        parsed.render_settings.width = render_map.at("width"s).AsDouble();
        parsed.render_settings.height = render_map.at("height"s).AsDouble();
        parsed.render_settings.padding = render_map.at("padding"s).AsDouble();

        parsed.render_settings.line_width = render_map.at("line_width"s).AsDouble();
        parsed.render_settings.stop_radius = render_map.at("stop_radius"s).AsDouble();

        parsed.render_settings.bus_label_font_size = render_map.at("bus_label_font_size"s).AsInt();

        auto bus_label_offset_arr = render_map.at("bus_label_offset"s).AsArray();
        parsed.render_settings.bus_label_offset[0] = bus_label_offset_arr[0].AsDouble();
        parsed.render_settings.bus_label_offset[1] = bus_label_offset_arr[1].AsDouble();

        parsed.render_settings.stop_label_font_size = render_map.at("stop_label_font_size"s).AsInt();

        auto stop_label_offset_arr = render_map.at("stop_label_offset"s).AsArray();
        parsed.render_settings.stop_label_offset[0] = stop_label_offset_arr[0].AsDouble();
        parsed.render_settings.stop_label_offset[1] = stop_label_offset_arr[1].AsDouble();

        parsed.render_settings.underlayer_color = getColorFromJsonNode(render_map.at("underlayer_color"s));

        parsed.render_settings.underlayer_width = render_map.at("underlayer_width"s).AsDouble();

        auto color_palette_arr = render_map.at("color_palette"s).AsArray();
        for (const auto& it : color_palette_arr) {
            (parsed.render_settings.color_palette).push_back(getColorFromJsonNode(it));
        }

        parsed.render_settings.height = render_map.at("height"s).AsDouble();
    }

    // --- parse requests --- //
    if (stats_dict_it != root_dict.end()) {

        for (const auto& it : stats_dict_it->second.AsArray()) {

            const auto& entry_dict = it.AsDict();

            Stat request;
            request.id = entry_dict.at("id").AsInt();
            std::string request_type = entry_dict.at("type").AsString();

            if (request_type == "Route"s) {
                request.type = RequestType::ROUTE;

                const auto from_it = entry_dict.find("from"s);
                const auto to_it = entry_dict.find("to"s);

                if (from_it != entry_dict.end() && to_it != entry_dict.end()) {
                    request.key_values["from"s] = from_it->second.AsString();
                    request.key_values["to"s] = to_it->second.AsString();
                }

                parsed.queries.push_back(std::move(request));
            }

            // "Stop" and "Bus" requests have similar structure
            if (request_type == "Stop"s) {
                request.type = RequestType::STOP;
                const auto payload_it = entry_dict.find("name"s);
                if (payload_it != entry_dict.end()) {
                    request.key_values["name"s] = payload_it->second.AsString();
                }

                parsed.queries.push_back(std::move(request));
            }

            if (request_type == "Bus"s) {
                request.type = RequestType::BUS;
                const auto payload_it = entry_dict.find("name"s);
                if (payload_it != entry_dict.end()) {
                    request.key_values["name"s] = payload_it->second.AsString();
                }

                parsed.queries.push_back(std::move(request));
            }

            if (request_type == "Map"s) {
                request.type = RequestType::MAP;
                parsed.queries.push_back(std::move(request));
            }
        }
    }

    return parsed;
}

svg::Color getColorFromJsonNode(const json::Node& node) {
    if (node.IsString()) {
        return node.AsString();
    }

    auto& arr = node.AsArray();

    if (arr.size() == 3) {
        return svg::Rgb{ static_cast<uint8_t>(arr[0].AsInt()), static_cast<uint8_t>(arr[1].AsInt()), static_cast<uint8_t>(arr[2].AsInt()) };
    }

    return svg::Rgba{ static_cast<uint8_t>(arr[0].AsInt()), static_cast<uint8_t>(arr[1].AsInt()), static_cast<uint8_t>(arr[2].AsInt()), arr[3].AsDouble() };
}