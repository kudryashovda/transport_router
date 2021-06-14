#include "graph.h"
#include "json_reader.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_router.h"

using namespace std;

int main() {
    tc::TransportCatalogue transport_catalogue;

    // Parse json input data
    Parsed_Inputs_Queries parsed_inputs_queries = ParseJson(LoadJSON(cin));

    transport_catalogue.FillTransportBase(parsed_inputs_queries.stops, parsed_inputs_queries.buses);

    // Init Graph
    graph::DirectedWeightedGraph<double> routes_graph(transport_catalogue.GetAllStopsCount()); // one vertex por one Stop

    // Prepare the graph to be filled with transport base 
    Transport_router transport_router(routes_graph, transport_catalogue, parsed_inputs_queries.routing_settings);
    transport_router.CreateGraph();

    // Handle the graph
    graph::Router<double> router(routes_graph);

    // Set SVG renderer
    MapRenderer map_renderer(parsed_inputs_queries.render_settings);

    // Handle requests
    RequestHandler requestHandler(transport_catalogue, map_renderer, router, transport_router);

    // --- process queries and generate output json array node --- //
    json::Array json_answer_array;

    for (const auto& request : parsed_inputs_queries.queries) {

        switch (request.type)
        {
        case RequestType::ROUTE:
            json_answer_array.push_back(std::move(GetRouteNode(request, requestHandler)));
            break;
        case RequestType::BUS:
            json_answer_array.push_back(std::move(GetBusInfoNode(request, requestHandler)));
            break;        
        case RequestType::STOP:
            json_answer_array.push_back(std::move(GetBusesListNode(request, requestHandler)));
            break;
        case RequestType::MAP:
            json_answer_array.push_back(std::move(GetTransportMapNode(request, requestHandler)));
            break;                    
        default:
            break;
        }
    }

    json::Print(json::Document{ json_answer_array }, std::cout);

    return 0;
}
