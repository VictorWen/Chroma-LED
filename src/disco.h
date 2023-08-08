#ifndef DISCO_H
#define DISCO_H

#include <cinttypes>
#include <memory>

#include <httpserver.hpp>

#include "chroma.h"

#define PORT 12345
#define DISCOVER_PORT 12346
#define DISCOVER_FOUND "DISCO FOUND\n"

struct DiscoPacket { 
    uint32_t start;
    uint32_t end;
    uint32_t n_frames;
    vec4 data[4000];
};

struct DiscoConfig {
    std::string test;
};

struct DiscoHardwareData {
    std::string controllerID;
    std::string device;
    int discoVersion;
    unsigned long address;
};

class DiscoMaster {
    public:
        virtual int write(const std::vector<vec4>& pixels) = 0;
};

class DiscoConfigManager {
    public:
        virtual DiscoConfig get_config(std::string id) = 0;
        virtual DiscoConfig set_config(std::string id, DiscoConfig config) = 0;
        virtual void add_hardware_data(DiscoHardwareData data) = 0;
        virtual DiscoHardwareData get_hardware_data(std::string id) = 0;
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
        std::unordered_map<std::string, DiscoConfig> configs;
        std::unordered_map<std::string, DiscoHardwareData> hardware_data;
        httpserver::webserver ws;
        ConfigPostResource config_resource;
    public:
        HTTPConfigManager() : ws(httpserver::create_webserver(8080)), config_resource(this) { } // TODO: deal with ports
        DiscoConfig get_config(std::string id);
        DiscoConfig set_config(std::string id, DiscoConfig config);
        void add_hardware_data(DiscoHardwareData data);
        DiscoHardwareData get_hardware_data(std::string id);
        void start();
};

class UDPDisco : public DiscoMaster {
    private:
        std::unique_ptr<DiscoConfigManager> manager;
    public:
        UDPDisco(std::unique_ptr<DiscoConfigManager>&& manager) : manager(std::move(manager)) { }
        int write(const std::vector<vec4>& pixels);
};

class DiscoDiscoverer {
    private:
        DiscoConfigManager* manager;
    public:
        DiscoDiscoverer(DiscoConfigManager* manager) : manager(manager) { }
        int send_broadcast();
};

#endif