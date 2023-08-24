#include <sstream>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "web_server.hpp"

ChromaWebServer::ChromaWebServer(ChromaController& chroma, ChromaCLI& cli, DiscoConfigManager& disco, int port) : 
        chroma(chroma),
        cli(cli),
        disco(disco),
        ws(httpserver::create_webserver(port)) { 
    // PASS
}


void ChromaWebServer::add_POST_route(ChromaHTTPResource& resource) {
    resource.disallow_all();
    resource.set_allowing("POST", true);
    this->ws.register_resource(this->base_path + resource.get_path(), &resource);
}

void ChromaWebServer::start() {
    fprintf(stderr, "Starting HTTP server\n");

    this->add_POST_route(this->chroma_config_rsc);
    this->add_POST_route(this->disco_config_rsc);
    this->add_POST_route(this->chroma_script_rsc);

    this->ws.start(false);
    fprintf(stderr, "HTTP server started\n");
}

std::shared_ptr<httpserver::http_response> ChromaWebServer::PostChromaConfig::render_POST(const httpserver::http_request &req) {
    return std::shared_ptr<httpserver::http_response>(new httpserver::string_response("Not Implemented", httpserver::http::http_utils::http_not_implemented));
}

std::shared_ptr<httpserver::http_response> ChromaWebServer::PostDiscoConfig::render_POST(const httpserver::http_request &req) {
    if (req.content_too_large())
        return std::shared_ptr<httpserver::http_response>(new httpserver::string_response("ERROR: Content too long")); // TODO: error handling
    
    std::string_view json_body = req.get_content();
    // fprintf(stderr, "GOT BODY: %s\n ==END== \n", json_body.data());

    json data = json::parse(json_body);
    // fprintf(stderr, "GOT DATA: %s\n ==END== \n", data.dump(2).c_str());

    auto config = data.get<DiscoConfig>();
    this->manager.set_config(config.controllerID, config);

    fprintf(stderr, "Updated config for %s\n", config.controllerID.c_str());

    return std::shared_ptr<httpserver::http_response>(new httpserver::string_response("Got config for " + config.controllerID));
}

std::shared_ptr<httpserver::http_response> ChromaWebServer::PostChromaScript::render_POST(const httpserver::http_request &req) {
    if (req.content_too_large())
        return std::shared_ptr<httpserver::http_response>(new httpserver::string_response("ERROR: Content too long")); // TODO: error handling

    std::string input(req.get_content());
    std::stringstream output;
    int result = -1;

    try {
        result = this->cli.process_text(input, output);
    } catch (ParseException& e) {
        fprintf(stderr, "Error: %s\n", e.what());
        return std::shared_ptr<httpserver::http_response>(new httpserver::string_response(e.what(), httpserver::http::http_utils::http_bad_request));
    }
    fprintf(stderr, "Got script request with result %d: %s\n", result, input.c_str());
 
    if (result == PARSE_GOOD)
        return std::shared_ptr<httpserver::http_response>(new httpserver::string_response("Parsed successfully"));
    else if (result == PARSE_CONTINUE)
        return std::shared_ptr<httpserver::http_response>(
            new httpserver::string_response("Parse Incomplete: awaiting more input")
        );
    return std::shared_ptr<httpserver::http_response>(new httpserver::string_response(output.str(), httpserver::http::http_utils::http_bad_request));
}
