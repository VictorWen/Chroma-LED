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
    std::string test;
};

class DiscoMaster {
    public:
        virtual int write(const std::vector<vec4>& pixels) = 0;
};

class DiscoConfigManager {
    public:
        virtual DiscoConfig get_config() = 0;
        virtual DiscoConfig set_config(std::string id, DiscoConfig config) = 0;
};

class HTTPConfigManager;

class ConfigPostResource : public httpserver::http_resource {
    private:
        HTTPConfigManager* manager;
    public:
        ConfigPostResource(HTTPConfigManager* manager) : manager(manager) { }
        std::shared_ptr<httpserver::http_response> render_POST(const httpserver::http_request& req);
};

class HTTPConfigManager : public DiscoConfigManager {
    private:
        DiscoConfig config;
        httpserver::webserver ws;
        ConfigPostResource config_resource;
    public:
        HTTPConfigManager() : ws(httpserver::create_webserver(8080)), config_resource(this) {} // TODO: deal with ports
        DiscoConfig get_config();
        DiscoConfig set_config(std::string id, DiscoConfig config);
        void start();
};

class UDPDisco : public DiscoMaster {
    private:
        std::unique_ptr<DiscoConfigManager> manager;
    public:
        UDPDisco(std::unique_ptr<DiscoConfigManager>&& manager) : manager(std::move(manager)) { }
        int write(const std::vector<vec4>& pixels);
};

#endif