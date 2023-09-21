// #include <bits/stdc++.h>
// #include <stdlib.h>
// #include <string.h>
#include <sys/types.h>
#include <string>
#include <string_view>
#include <chrono>

#ifdef _WIN32
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "disco.hpp"

struct Packet {
    sockaddr_in addr;
    size_t length;
    char data[PACKET_MAX];
};

void to_json(json& j, const DiscoConfig& config) {
    j = json{
        {"controllerID", config.controllerID},
        {"device", config.device},
        {"discoVersion", config.discoVersion},
        {"address", config.address}
    };
}

void from_json(const json& j, DiscoConfig& config) {
    j.at("controllerID").get_to(config.controllerID);
    j.at("device").get_to(config.device);
    j.at("discoVersion").get_to(config.discoVersion);
    if (j.count("address") > 0)
        j.at("address").get_to(config.address);
}

size_t write_packet(const std::vector<vec4>& pixels, char buffer[4096]) {
    // Fill packet information
    DiscoPacket packet;
    packet.start = 0; // TODO: start, end, n_frames should be calculated dynamically from pixel length
    packet.end = 150;
    packet.n_frames = 1;
    // std::copy(pixels.begin(), pixels.end(), packet.data);
    size_t packet_len = (packet.end - packet.start) * packet.n_frames * 4 + 16;

    if (packet_len > PACKET_MAX) {
        fprintf(stderr, "Packet data to large %lld, buffer overflow\n", packet_len);
        return -1;
    }

    for (size_t i = 0; i < pixels.size(); i++) {
        const vec4 pixel = pixels[i];
        packet.data[i * 4 + 0] = pixel.x * 255;
        packet.data[i * 4 + 1] = pixel.y * 255;
        packet.data[i * 4 + 2] = pixel.z * 255;
        packet.data[i * 4 + 3] = pixel.w * 255;
    }

    memcpy(buffer, "LEDA", 4); // Set packet type
    memcpy(buffer + 4, &packet, packet_len - 4);

    return packet_len;
}

sockaddr_in get_addr(const std::unique_ptr<DiscoConfigManager>& manager, std::string device_name) {
    DiscoConfig config = manager->get_config(device_name);
    
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(config.address.c_str());

    return addr;
}

void print_error(const char* error_msg) {
#ifdef _WIN32
    fprintf(stderr, "%s %d\n", error_msg, WSAGetLastError());
#else
    perror(error_msg);
#endif
}

int fill_fd_set(const std::vector<int> sockets, fd_set* set) {
    int max_sock = -1;
    FD_ZERO(set);
    for (int sock : sockets) {
        if (sock > max_sock)
            max_sock = sock;
        FD_SET(sock, set);
    }
    return max_sock + 1;
}

int multi_send(const std::vector<int>& sockets, const std::vector<Packet>& packets) {
    int num_socks;
    fd_set sock_set;
    auto it = packets.begin();
    while (it != packets.end()) {
        int nfds = fill_fd_set(sockets, &sock_set);
        num_socks = select(nfds, NULL, &sock_set, NULL, NULL);
        if (num_socks < 0) {
            print_error("select got an error");
            return -1;
        }

        if (num_socks > 0) {
            for (size_t i = 0; i < sockets.size() && it != packets.end(); i++) {
                int sock = sockets[i];
                if (!FD_ISSET(sock, &sock_set))
                    continue;
                
                int send_result = sendto(sock, it->data, it->length, 0, (sockaddr*) &it->addr, (int) sizeof(it->addr));
                if (send_result < 0) {
                    print_error("Heartbeat: Sending got an error");
                    continue;
                }

                it++;
            }
        }
    }
    return 0;
}

//! TEMP FUNCTION
std::string get_heartbeat_value(const DiscoHeartbeat& heartbeat, const std::string& key) {
    std::stringstream ss(heartbeat.data);
    std::string to;

    if (heartbeat.data.size() > 0) {
        while(std::getline(ss,to,'\n')){
            if (to.rfind(key, 0) == 0)
                return to.substr(key.size() + 1);
        }
    }
    return "";
}

int DiscoHeartbeat::response_from_buffer(char buffer[PACKET_MAX], size_t packet_len, DiscoHeartbeat& heartbeat) {
    if (packet_len < 16)
        return -1;
    std::string_view packet_type(buffer, 4);
    if (packet_type != "HRBB")
        return -2;

    unsigned long long timestamp;
    unsigned int length;
    memcpy(&timestamp, buffer + 4, 8);
    memcpy(&length, buffer + 12, 4);
    
    if (packet_len < 16 + length)
        return -1;

    heartbeat.timestamp = timestamp;
    heartbeat.data = std::string(buffer + 16, length);
    return 0;
}

size_t DiscoHeartbeat::write_to_buffer(char buffer[PACKET_MAX]) const
{
    size_t length = this->data.size();
    const char* data = this->data.c_str();
    
    size_t packet_len = 16 + length;
    if (packet_len > PACKET_MAX) {
        fprintf(stderr, "Packet data to large %lld, buffer overflow\n", packet_len);
        return -1;
    }

    memcpy(buffer + 0, "HRBA", 4);
    memcpy(buffer + 4, &this->timestamp, 8);
    memcpy(buffer + 12, &length, 4);
    memcpy(buffer + 16, data, length);

    return packet_len;
}

DiscoConnection::DiscoConnection(const DiscoConnection &other) :
    name(other.name), 
    status(other.status), 
    sent_heartbeats(other.sent_heartbeats), 
    max_heartbeats(other.max_heartbeats) { }

const DiscoHeartbeat DiscoConnection::get_heartbeat()
{
    DiscoHeartbeat heartbeat(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count(),
        "name=disco.local\n"// TODO: temp name
    );

    if (this->sent_heartbeats.size() >= this->max_heartbeats) {
        // TODO: too many heartbeats, timeout
        this->status = DISCONNECTED;
    }

    this->sent_heartbeats.push_back(heartbeat);
    return heartbeat;
}

void DiscoConnection::recv_heartbeat(const DiscoHeartbeat& heartbeat) {
    auto it = this->sent_heartbeats.begin();
    while (it != this->sent_heartbeats.end() && heartbeat.timestamp > it->timestamp) {
        it++;
    }

    if (it == this->sent_heartbeats.end() || 
            it->timestamp != heartbeat.timestamp ||
            it->data != heartbeat.data) {
        // TODO: invalid heartbeat
    }

    this->sent_heartbeats.erase(this->sent_heartbeats.begin(), it);
    this->status = CONNECTED;
}

std::shared_ptr<httpserver::http_response> ConfigPostResource::render_POST(const httpserver::http_request& req) {
    if (req.content_too_large())
        return std::shared_ptr<httpserver::http_response>(new httpserver::string_response("ERROR: Content too long")); // TODO: error handling
    
    std::string_view json_body = req.get_content();
    // fprintf(stderr, "GOT BODY: %s\n ==END== \n", json_body.data());

    json data = json::parse(json_body);
    // fprintf(stderr, "GOT DATA: %s\n ==END== \n", data.dump(2).c_str());

    auto config = data.get<DiscoConfig>();
    this->manager->set_config(config.controllerID, config);

    fprintf(stderr, "Updated config for %s\n", config.controllerID.c_str());

    return std::shared_ptr<httpserver::http_response>(new httpserver::string_response("Got config for " + config.controllerID));
}

DiscoConfig HTTPConfigManager::get_config(std::string id) {
    if (this->configs.count(id) > 0)
        return this->configs[id];
    else
        return DiscoConfig {0};
}

DiscoConfig HTTPConfigManager::set_config(std::string id, DiscoConfig config) {
    this->configs[id] = config;
    return this->configs[id];
}

void HTTPConfigManager::start() {
    fprintf(stderr, "Starting HTTP server\n");

    this->config_resource.disallow_all();
    this->config_resource.set_allowing("POST", true);

    this->ws.register_resource("/config", &(this->config_resource));
    this->ws.start(false);
    fprintf(stderr, "HTTP server started\n");
}

void HTTPConfigManager::wait_for_any_config() {
    while (this->configs.empty()) { // TODO: include TCP alternative?
        usleep(1000);
    }
}

UDPDisco::UDPDisco(std::unique_ptr<DiscoConfigManager> &&manager, size_t num_sockets) : manager(std::move(manager))
{
    // Create socket
    this->disco_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (this->disco_socket == -1) {
        print_error("socket failed with error");
        exit(1);
    }

    for (size_t i = 0; i < num_sockets; i++) {
        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == -1) {
            print_error("socket failed with error");
            exit(1);
        }
        this->sockets.push_back(sock);
    }
}

UDPDisco::~UDPDisco() {
#ifdef _WIN32
    closesocket(this->disco_socket);
#else
    close(this->disco_socket);
#endif

    for (int sock : this->sockets) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
    }
}

void UDPDisco::start_heartbeats() {
    this->heart_beating = true;
    this->heart_thread = std::thread([&](){ this->run_heartbeat(); });
}

void UDPDisco::stop_heartbeats() {
    this->heart_beating = false;
    this->heart_thread.join();
}

void UDPDisco::add_connection(const std::string& device_name, DiscoConnection connection) {
    if (this->connections.count(device_name) == 0)
        this->connections.emplace(device_name, connection);
}

const DiscoConnection &UDPDisco::get_connection(const std::string& device_name) const {
    // TODO: error handling
    return this->connections.at(device_name);
}

void UDPDisco::purge_connections() {
    std::vector<std::string> not_found;
    for (auto& pair : this->connections) {
        if (pair.second.get_status() == NOT_FOUND)
            not_found.push_back(pair.first);
    }
    for (auto& name : not_found) {
        this->connections.erase(name);
    }
}

std::vector<std::string> UDPDisco::get_connection_names() const
{
    std::vector<std::string> names;
    for (auto& pair : this->connections) {
        names.push_back(pair.first);
    }
    return names;
}

int UDPDisco::write(const std::string &id, const std::vector<vec4> &pixels)
{
    sockaddr_in server_addr = get_addr(this->manager, id);

    // Write packet to socket
    char send_buffer[PACKET_MAX];
    int packet_len = write_packet(pixels, send_buffer);
    if (packet_len < 0) {
        return 1;
    }

    int send_result = sendto(this->disco_socket, send_buffer, packet_len, 0, (sockaddr*) &server_addr, (int) sizeof(server_addr));
    if (send_result < 0) {
        print_error("Sending got an error");
        return 1;
    }

    return 0;
}

void UDPDisco::run_heartbeat() { // TODO: refactor
    while (this->heart_beating) {
        // Send heartbeats
        std::vector<Packet> packets;
        for (auto& connection_pair : this->connections) {
            Packet packet;
            std::string device_name = connection_pair.first;
            DiscoConnection& connection = connection_pair.second;
            
            packet.addr = get_addr(this->manager, device_name);
            DiscoHeartbeat heartbeat = connection.get_heartbeat();
            packet.length = heartbeat.write_to_buffer(packet.data);
            packets.push_back(packet);
            //fprintf(stderr, "Sending heartbeat to device: %s, timestamp: %lld\n", device_name.c_str(), heartbeat.timestamp);
        }
        multi_send(this->sockets, packets);

        // TODO: extract into function
        // Listen for responses 
        int num_socks;
        fd_set sock_set;
        int nfds = fill_fd_set(this->sockets, &sock_set);
        timeval timeout = {0};
        num_socks = select(nfds, &sock_set, NULL, NULL, &timeout);
        
        if (num_socks < 0) {
            print_error("Heartbeat: read select got an error");
        }
        else if (num_socks > 0) {
            for (int sock : this->sockets) {
                if (!FD_ISSET(sock, &sock_set))
                    continue;
                sockaddr_in server_addr;
                socklen_t addr_len = sizeof(server_addr);
                char recv_buffer[PACKET_MAX];
                int recv_result = recvfrom(sock, recv_buffer, PACKET_MAX, 0, (sockaddr*) &server_addr, &addr_len);
                if (recv_result < 0)
                    print_error("Heartbeat: recvfrom got an error");
                else if (recv_result > 0) {
                    DiscoHeartbeat heartbeat;
                    int result = DiscoHeartbeat::response_from_buffer(recv_buffer, recv_result, heartbeat);
                    if (result == 0) {
                        std::string name = get_heartbeat_value(heartbeat, "name");
                        //fprintf(stderr, "Got heartbeat response for device: %s, timestamp: %lld\n", name.c_str(), heartbeat.timestamp);
                        if (this->connections.count(name) > 0)
                            this->connections.at(name).recv_heartbeat(heartbeat);
                    } else {
                        fprintf(stderr, "Parsing heartbeat failed with code %d\n", result);
                    }
                }
            }
        }

        sleep(1);
    }
}

int on_mDNS_service_found(int sock, const struct sockaddr* from, size_t addrlen,
        mdns_entry_type_t entry, uint16_t query_id, uint16_t rtype,
        uint16_t rclass, uint32_t ttl, const void* data, size_t size,
        size_t name_offset, size_t name_length, size_t record_offset,
        size_t record_length, void* user_data) {
    const size_t buff_size = 256;
    char buffer[buff_size];

    fprintf(stderr, "GOT MDNS RESPONSE\n");
    
    mdns_string_t entry_str = mdns_string_extract(data, size, &name_offset, buffer, buff_size);
    std::string entry_name(entry_str.str, entry_str.length);
    if (entry_name.back() == '.')
        entry_name.pop_back();

    auto manager = static_cast<mDNSRecordManager*>(user_data);

    if (rtype == MDNS_RECORDTYPE_PTR) {
        mdns_string_t name_str = mdns_record_parse_ptr(data, size, record_offset, record_length,
            buffer, buff_size);
        std::string name(name_str.str, name_str.length);
        if (name.back() == '.')
            name.pop_back();
        manager->process_PTR(entry_name, name);
    }
    else if (rtype == MDNS_RECORDTYPE_SRV) {
        mdns_record_srv_t srv = mdns_record_parse_srv(data, size, record_offset, record_length,
            buffer, buff_size);
        std::string name(srv.name.str, srv.name.length);
        if (name.back() == '.')
            name.pop_back();
        manager->process_SRV(entry_name, name, srv.port);
    }
    else if (rtype == MDNS_RECORDTYPE_TXT) {
        const size_t txt_buffer_size = 128;
        mdns_record_txt_t txt_buffer[txt_buffer_size];
        size_t parsed = mdns_record_parse_txt(data, size, record_offset, record_length, txt_buffer,
		                                      txt_buffer_size);
		for (size_t i = 0; i < parsed; i++) {
            mdns_record_txt_t text_record = txt_buffer[i];
			if (text_record.value.length > 0) {
                std::string key(text_record.key.str, text_record.key.length);
                std::string value(text_record.value.str, text_record.value.length);
                manager->process_TXT(entry_name, key, value);
			} else {
				std::string key(text_record.key.str, text_record.key.length);
                manager->process_TXT(entry_name, key, "NULL");
			}
		}
    }
    else if (rtype == MDNS_RECORDTYPE_A) {
        struct sockaddr_in addr;
		mdns_record_parse_a(data, size, record_offset, record_length, &addr);
        inet_ntop(AF_INET, &(addr.sin_addr), buffer, buff_size);
        std::string ip_addr(buffer);
        manager->process_A(entry_name, ip_addr);
    }
    else if (rtype == MDNS_RECORDTYPE_AAAA) {
        struct sockaddr_in6 addr;
		mdns_record_parse_aaaa(data, size, record_offset, record_length, &addr);
        inet_ntop(AF_INET6, &(addr.sin6_addr), buffer, buff_size);
        std::string ip_addr(buffer);
        manager->process_A(entry_name, ip_addr);
    }

    return 0;
}

int DiscoDiscoverer::mDNS_auto_discover() {
    // Create socket
    int sock = -1;
    sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = inet_addr("192.168.0.175"); //! TEMP FIX
    saddr.sin_port = htons(MDNS_PORT);
    sock = mdns_socket_open_ipv4(&saddr); // TODO: open a socket for every interface
    // sock = mdns_socket_open_ipv4(0);
    if (sock < 0) {
        print_error("socket failed with error");
        return 1;
    }

    fprintf(stderr, "Starting mDNS auto discovery\n");

    const size_t buff_size = 2048;
    void* buffer[buff_size]; // hope that mdns library deals with buffer overflows...
    std::string query = "_disco._udp.local";
    if (mdns_query_send(sock, MDNS_RECORDTYPE_PTR, query.c_str(), query.size(), buffer, buff_size, 0) < 0) {
        print_error("mdns query failed");
        return 1;
    }

    // Collect responses
    mDNSRecordManager builder;
    const double timeout = 5;
    auto timer = std::chrono::high_resolution_clock::now();
    while (std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - timer).count() < timeout * 1e3) {
        mdns_query_recv(sock, buffer, buff_size, on_mDNS_service_found, &builder, 0);
    }
    std::vector<std::string> names = builder.get_configs_to(this->manager);
    // fprintf(stderr, "GOT config: %s\n", json(this->manager.get_config("disco.local")).dump(2).c_str());

    // Create and test connections
    for (auto& name : names) {
        DiscoConnection connection(name);
        this->master.add_connection(name, connection);
    }
    fprintf(stderr, "Testing connections\n");
    this->master.start_heartbeats();
    sleep(timeout);
    this->master.stop_heartbeats();
    this->master.purge_connections();

    fprintf(stderr, "Finished mDNS auto discovery\n");
    fprintf(stderr, "FOUND connections:\n");
    for (auto& name : this->master.get_connection_names()) {
        fprintf(stderr, "\t%s\n", name.c_str());
    }

    mdns_socket_close(sock);
    return 0;
}

//! NOTE: requires router to support broadcast packets
//! DEPRECATED
int DiscoDiscoverer::broadcast_auto_discover() { // TODO: add support for multicast

    // TODO: deal with timeouts

    fprintf(stderr, "Starting auto-discovery\n");

    // Create socket
    int client_socket = -1;
    client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client_socket == -1) {
        print_error("socket failed with error");
        return 1;
    }

    char flags = 1;

    // Set socket flags
    if (setsockopt(client_socket, SOL_SOCKET, SO_BROADCAST, &flags, sizeof(flags)) < 0)
        print_error("setsockopt failed with error");

    // Define address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DISCOVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_BROADCAST;

    // Write packet to socket
    char send_buffer[4096] = "DISCO DISCOVER\n"; 
    int send_result = sendto(client_socket, send_buffer, strlen(send_buffer), 0, (struct sockaddr *) & server_addr, (int)sizeof(server_addr));
    if (send_result < 0) {
        print_error("Sending got an error");
        return 1;
    }

    // Get response
    int bytes_received;
    char server_buf[1025]; // TODO: BUFFER OVERFLOW!!!!
    int server_buf_len = 1024;

    struct sockaddr_in sender_addr;
    int sender_addr_size = sizeof (sender_addr);

    bytes_received = recvfrom(client_socket, server_buf, server_buf_len, 0, (struct sockaddr *) & sender_addr, &sender_addr_size);
    if (bytes_received < 0) {
        print_error("recvfrom failed with error");
        return 1;
    }
    server_buf[bytes_received] = '\0';

    fprintf(stderr, "Got response: %s\n", server_buf);

    // Save hardware info
    std::string_view response(server_buf);
    if (response.substr(0, strlen(DISCOVER_FOUND)) != DISCOVER_FOUND) {
        fprintf(stderr, "Unexpected response\n");
        return 1;
    }
    
    json json_data = json::parse(response.substr(strlen(DISCOVER_FOUND)));
    DiscoConfig config = json_data.get<DiscoConfig>();
    inet_ntop(AF_INET, &(sender_addr.sin_addr), send_buffer, 1000);
    config.address = std::string(send_buffer);

    this->manager.set_config(config.controllerID, config);

    // TODO: user prompt to connect

    // Return connect
    strcpy(send_buffer, "DISCO CONNECT\n{\"discoVersion\":1,\"configProtocol\":\"HTTP\",\"dataProtocol\":\"UDP\"}"); // TODO: generalize response data
    send_result = sendto(client_socket, send_buffer, strlen(send_buffer), 0, (struct sockaddr *) & sender_addr, sender_addr_size);
    if (send_result < 0) {
        print_error("Sending got an error");
        return 1;
    }

    // TODO: Disco heartbeat

    // Wait for READY message
    bytes_received = recvfrom(client_socket, server_buf, server_buf_len, 0, (struct sockaddr *) & sender_addr, &sender_addr_size);
    if (bytes_received < 0) {
        print_error("recvfrom failed with error");
        return 1;
    }
    server_buf[bytes_received] = '\0';

    response = std::string_view(server_buf);
    if (response != DISCOVER_READY) {
        fprintf(stderr, "Unexpected response\n");
        return 1;
    }

    fprintf(stderr, "Finished auto-discovery scan\n");

#ifdef _WIN32
    closesocket(client_socket);
#else
    close(client_socket);
#endif
    return 0;
}

void mDNSRecordManager::process_PTR(const std::string &query_name, const std::string &record_name) {
    if (this->records.count(record_name) == 0)
        this->records[record_name] = std::make_unique<mDNSRecord>();;
    this->records[record_name]->query_name = query_name;
}

void mDNSRecordManager::process_SRV(const std::string &record_name, const std::string &host_name, int port) {
    if (this->records.count(record_name) == 0)
        this->records[record_name] = std::make_unique<mDNSRecord>();;
    this->records[record_name]->host_name = host_name;
    this->records[record_name]->port = port;
}

void mDNSRecordManager::process_TXT(const std::string &record_name, const std::string &key, const std::string &value) {
    if (this->records.count(record_name) == 0)
        this->records[record_name] = std::make_unique<mDNSRecord>();;
    this->records[record_name]->text[key] = value;
}

void mDNSRecordManager::process_A(const std::string &host_name, const std::string &ip_addr) {
    this->ip4_table[host_name] = ip_addr;
}

void mDNSRecordManager::process_AAAA(const std::string &host_name, const std::string &ip_addr) {
    this->ip6_table[host_name] = ip_addr;
}

std::vector<std::string> mDNSRecordManager::get_configs_to(DiscoConfigManager& manager) {
    std::vector<std::string> names;
    for (auto& record_pair : this->records) {
        auto& record = record_pair.second;
        if (record->host_name != "" && record->text.count("version") > 0) {
            fprintf(stderr, "Adding record %s\n", record->host_name.c_str());
            manager.set_config(record->host_name, {
                record->host_name,
                record->text.count("device") > 0 ? record->text["device"] : "unknown",
                stoi(record->text["version"]),
                this->ip4_table[record->host_name]
            }); // TODO: provide more info to config
            names.push_back(record->host_name);
        }
    }
    return names;
}
