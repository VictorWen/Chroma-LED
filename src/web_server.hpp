/*
    Module for hosting a web server backend that
    1. Configures Chroma
    2. Configures Disco
    3. Sends Commands to Chroma
*/

#ifndef CHROMA_WEB_SERVER_H
#define CHROMA_WEB_SERVER_H

#include <string>

#include <httpserver.hpp>

#include "chroma.hpp"
#include "chroma_cli.hpp"
#include "disco.hpp"

class ChromaHTTPResource : public httpserver::http_resource {
    private:
        std::string path;
    public:
        ChromaHTTPResource(const std::string& path) { this->path = path; }
        const std::string& get_path() { return this->path; }
};

class ChromaWebServer {
    private:
        class PostChromaConfig : public ChromaHTTPResource {
            private:
                ChromaController& controller;
            public:
                PostChromaConfig(ChromaController& controller) : ChromaHTTPResource("/chroma/config"), controller(controller) { }
                std::shared_ptr<httpserver::http_response> render_POST(const httpserver::http_request& req);
        };

        class PostDiscoConfig : public ChromaHTTPResource {
            private:
                DiscoConfigManager& manager;
            public:
                PostDiscoConfig(DiscoConfigManager& manager) : ChromaHTTPResource("/disco/config"), manager(manager) { }
                std::shared_ptr<httpserver::http_response> render_POST(const httpserver::http_request& req);
        }; 

        class PostChromaScript : public ChromaHTTPResource {
            private:
                ChromaCLI& cli;
            public:
                PostChromaScript(ChromaCLI& cli) : ChromaHTTPResource("/chroma/script"), cli(cli) { }
                std::shared_ptr<httpserver::http_response> render_POST(const httpserver::http_request& req);
        };
        
        ChromaController& chroma;
        ChromaCLI& cli;
        DiscoConfigManager& disco;

        std::string base_path = "/api";
        httpserver::webserver ws;
        PostChromaConfig chroma_config_rsc = PostChromaConfig(this->chroma);
        PostDiscoConfig disco_config_rsc = PostDiscoConfig(this->disco);
        PostChromaScript chroma_script_rsc = PostChromaScript(this->cli);

        void add_POST_route(ChromaHTTPResource& resource);
    public:
        ChromaWebServer(ChromaController& chroma, ChromaCLI& cli, DiscoConfigManager& disco, int port);
        void start();
};


#endif