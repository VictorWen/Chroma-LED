#ifndef DISCO_H
#define DISCO_H

#include <cinttypes>
#include <memory>

#include <httpserver.hpp>

#include <mdns.h>

#include "chroma.h"

#define PORT 12345
#define DISCOVER_PORT 12346
#define DISCOVER_FOUND "DISCO FOUND\n"
#define DISCOVER_READY "DISCO READY\n"

struct DiscoPacket { 
    uint32_t start;
    uint32_t end;
    uint32_t n_frames;
    vec4 data[4000];
};

struct DiscoConfig {
    std::string controllerID;
    std::string device;
    int discoVersion;
    std::string address;
};

class DiscoMaster {
    public:
        virtual int write(const std::string& id, const std::vector<vec4>& pixels) = 0;
};

class DiscoConfigManager {
    public:
        virtual DiscoConfig get_config(std::string id) = 0;
        virtual DiscoConfig set_config(std::string id, DiscoConfig config) = 0;
        virtual bool has_config(std::string id);
};

class HTTPConfigManager;

class ConfigPostResource : public httpserver::http_resource {
    private:
        HTTPConfigManager* manager;
    public:
        ConfigPostResource(HTTPConfigManager* manager) : manager(manager) { }
        std::shared_ptr<httpserver::http_response> render_POST(const httpserver::http_request& req);
};

class HTTPConfigManager : public DiscoConfigManager { //TODO: send commands via HTTP, may require refactor
    private:
        std::unordered_map<std::string, DiscoConfig> configs;
        httpserver::webserver ws;
        ConfigPostResource config_resource;
    public:
        HTTPConfigManager() : ws(httpserver::create_webserver(8080)), config_resource(this) { } // TODO: deal with ports
        DiscoConfig get_config(std::string id);
        DiscoConfig set_config(std::string id, DiscoConfig config);
        bool has_config(std::string id) { return this->configs.count(id) > 0; }
        void start();
        void wait_for_any_config();
};

class UDPDisco : public DiscoMaster {
    private:
        std::unique_ptr<DiscoConfigManager> manager;
    public:
        UDPDisco(std::unique_ptr<DiscoConfigManager>&& manager) : manager(std::move(manager)) { }
        int write(const std::string& id, const std::vector<vec4>& pixels);
};

class DiscoDiscoverer {
    private:
        DiscoConfigManager* manager;
        // int on_mDNS_service_found(int sock, const struct sockaddr* from, size_t addrlen,
        //                                mdns_entry_type_t entry, uint16_t query_id, uint16_t rtype,
        //                                uint16_t rclass, uint32_t ttl, const void* data, size_t size,
        //                                size_t name_offset, size_t name_length, size_t record_offset,
        //                                size_t record_length, void* user_data);
    public:
        DiscoDiscoverer(DiscoConfigManager* manager) : manager(manager) { }
        int send_mDNS_query();
        int send_broadcast();
};

#endif