#ifndef DISCO_H
#define DISCO_H

#include <cinttypes>
#include <memory>

#include <httpserver.hpp>

#include "chroma.h"

struct DiscoPacket { 
    uint32_t start;
    uint32_t end;
    uint32_t n_frames;
    vec4 data[4000];
};

struct DiscoConfig {
    int f;
};

class DiscoController {
    public:
        virtual int write(const std::vector<vec4>& pixels) = 0;
};

class DiscoConfigManager {
    public:
        virtual DiscoConfig getConfig() = 0;
};

class UDPDisco : public DiscoController {
    public:
        int write(const std::vector<vec4>& pixels);
};

class hello_world_resource : public httpserver::http_resource {
    public:
        std::shared_ptr<httpserver::http_response> render(const httpserver::http_request&) {
            fprintf(stderr, "GOT REQUEST\n");
            return std::shared_ptr<httpserver::http_response>(new httpserver::string_response("Hello, World!"));
        }
};

class HTTPConfigManager : DiscoConfigManager {
    private:
        DiscoConfig config;
        httpserver::webserver ws;
        hello_world_resource hwr;
    public:
        HTTPConfigManager() : ws(httpserver::create_webserver(8080)) {}
        DiscoConfig getConfig();
        void start();
};

#endif