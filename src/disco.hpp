#ifndef DISCO_H
#define DISCO_H

#include <cinttypes>
#include <memory>
#include <thread>

#include <httpserver.hpp>

#include <mdns.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "chroma.hpp" // TODO: should this dependency be reversed??

#define PACKET_MAX 4096
#define PORT 12345
#define DISCOVER_PORT 12346
#define DISCOVER_FOUND "DISCO FOUND\n"
#define DISCOVER_READY "DISCO READY\n"

struct DiscoPacket { 
    uint32_t start;
    uint32_t end;
    uint32_t n_frames;
    char data[4000];
};

struct DiscoConfig {
    std::string controllerID;
    std::string device;
    int discoVersion;
    std::string address;
};

void to_json(json& j, const DiscoConfig& config);
void from_json(const json& j, DiscoConfig& config);

enum DiscoConnectionStatus {
    AVAILABLE,
    CONNECTED,
    DISCONNECTED,
    NOT_FOUND
};

struct DiscoHeartbeat {
    uint64_t timestamp;
    std::string data;

    DiscoHeartbeat(unsigned long long timestamp=0, std::string data="") :
        timestamp(timestamp), data(data) { }
    static int response_from_buffer(const char buffer[PACKET_MAX], size_t packet_len, DiscoHeartbeat& heartbeat);
    size_t write_to_buffer(char buffer[PACKET_MAX]) const;
};

class DiscoConnection {
    private:
        std::string name;
        DiscoConnectionStatus status = NOT_FOUND;
        std::deque<DiscoHeartbeat> sent_heartbeats;
        size_t max_heartbeats = 10;
    public:
        DiscoConnection(const std::string& device_name) : name(device_name) { };
        DiscoConnection(const DiscoConnection& other);
        // ~DiscoConnection();
        const DiscoHeartbeat get_heartbeat();
        void recv_heartbeat(const DiscoHeartbeat& heartbeat);
        // void disconnect();
        // void connect();
        DiscoConnectionStatus get_status() { return this->status; };
        // int write(const std::vector<vec4>& pixels);
};

class DiscoMaster {
    public:
        virtual ~DiscoMaster() { }
        virtual void start_heartbeats() = 0;
        virtual void stop_heartbeats() = 0;
        virtual void add_connection(const std::string& device_name, DiscoConnection connection) = 0;
        virtual const DiscoConnection& get_connection(const std::string& device_name) const = 0;
        virtual void purge_connections() = 0;
        virtual std::vector<std::string> get_connection_names() const = 0;
        virtual int write(const std::string& id, const std::vector<vec4>& pixels) = 0;
};


class DiscoConfigManager {
    public:
        virtual DiscoConfig get_config(std::string id) = 0;
        virtual DiscoConfig set_config(std::string id, DiscoConfig config) = 0;
        virtual bool has_config(std::string id) = 0;
};

class DictionaryConfigManager : public DiscoConfigManager {
    private:
        std::unordered_map<std::string, DiscoConfig> dictionary;
    public:
        DiscoConfig get_config(std::string id) { return this->dictionary[id]; }
        DiscoConfig set_config(std::string id, DiscoConfig config) { this->dictionary[id] = config; return this->dictionary[id]; }
        bool has_config(std::string id) { return this->dictionary.count(id) > 0; }
};

class HTTPConfigManager;

class ConfigPostResource : public httpserver::http_resource {
    private:
        HTTPConfigManager* manager;
    public:
        ConfigPostResource(HTTPConfigManager* manager) : manager(manager) { }
        std::shared_ptr<httpserver::http_response> render_POST(const httpserver::http_request& req);
};

//? Disco should deal with low level delivery of data
//? HTTP Config management is more high level
//? Especially if it can send commands to Chroma controller
//? Thus, should move the manager to a different file, outside of Disco
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
}; // TODO: maybe delete/move

class UDPDisco : public DiscoMaster {
    private:
        std::unique_ptr<DiscoConfigManager> manager;
        std::unordered_map<std::string, DiscoConnection> connections;
        std::thread heart_thread;
        std::vector<int> sockets;
        int disco_socket;
        bool heart_beating = false;

        void run_heartbeat();
    public:
        UDPDisco(std::unique_ptr<DiscoConfigManager>&& manager, size_t num_sockets=4);
        ~UDPDisco();
        void start_heartbeats();
        void stop_heartbeats();
        void add_connection(const std::string& device_name, DiscoConnection connection);
        const DiscoConnection& get_connection(const std::string& device_name) const;
        void purge_connections();
        std::vector<std::string> get_connection_names() const;
        int write(const std::string& id, const std::vector<vec4>& pixels);
};

class DiscoDiscoverer {
    private:
        DiscoMaster& master;
        DiscoConfigManager& manager;
    public:
        DiscoDiscoverer(DiscoMaster& master, DiscoConfigManager& manager) : master(master), manager(manager) { }
        int mDNS_auto_discover();
        int broadcast_auto_discover();
};

class mDNSRecordManager {
    private:
        struct mDNSRecord {
            std::string query_name;
            std::string entry_name;
            std::string host_name;
            int port;
            std::unordered_map<std::string, std::string> text;
        };

        std::unordered_map<std::string, std::string> ip4_table;
        std::unordered_map<std::string, std::string> ip6_table;
        std::unordered_map<std::string, std::unique_ptr<mDNSRecord>> records;

    public:
        void process_PTR(const std::string& query_name, const std::string& record_name);
        void process_SRV(const std::string& record_name, const std::string& host_name, int port);
        void process_TXT(const std::string& record_name, const std::string& key, const std::string& value);
        void process_A(const std::string& host_name, const std::string& ip_addr);
        void process_AAAA(const std::string& host_name, const std::string& ip_addr);
        std::vector<std::string> get_configs_to(DiscoConfigManager& manager);
};

#endif