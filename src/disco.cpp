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
#else
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "disco.hpp"

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

int write_packet(const std::vector<vec4>& pixels, char buffer[4096]) {
    // Fill packet information
    DiscoPacket packet;
    packet.start = 0; // TODO: start, end, n_frames should be calculated dynamically from pixel length
    packet.end = 150;
    packet.n_frames = 1;
    // std::copy(pixels.begin(), pixels.end(), packet.data);
    int packet_len = (packet.end - packet.start) * packet.n_frames * 4 + 12;
    for (size_t i = 0; i < pixels.size(); i++) {
        const vec4 pixel = pixels[i];
        packet.data[i * 4 + 0] = pixel.x * 255;
        packet.data[i * 4 + 1] = pixel.y * 255;
        packet.data[i * 4 + 2] = pixel.z * 255;
        packet.data[i * 4 + 3] = pixel.w * 255;
    }

    memcpy(buffer, &packet, packet_len);

    return packet_len;
}

void print_error(const char* error_msg) {
#ifdef _WIN32
    fprintf(stderr, "%s %d\n", error_msg, WSAGetLastError());
#else
    perror(error_msg);
#endif
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

UDPDisco::UDPDisco(std::unique_ptr<DiscoConfigManager>&& manager) : manager(std::move(manager)) {
    // Create socket
    this->disco_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (this->disco_socket == -1) {
        print_error("socket failed with error");
        exit(1);
    }
}

UDPDisco::~UDPDisco() {
#ifdef _WIN32
    closesocket(this->disco_socket);
#else
    close(this->disco_socket);
#endif
}

int UDPDisco::write(const std::string &id, const std::vector<vec4> &pixels)
{
    // Get config
    DiscoConfig config = this->manager->get_config(id);

    // Define address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(config.address.c_str());

    // Write packet to socket
    char send_buffer[4096]; // TODO: BUFFER OVERFLOW!!!!
    int packet_len = write_packet(pixels, send_buffer);
    int send_result = sendto(this->disco_socket, send_buffer, packet_len, 0, (struct sockaddr *) & server_addr, (int)sizeof(server_addr));
    if (send_result < 0) {
        print_error("Sending got an error");
        return 1;
    }

    // // Get response
    // int bytes_received;
    // char server_buf[1025]; // TODO: BUFFER OVERFLOW!!!!
    // int server_buf_len = 1024;

    // struct sockaddr_in sender_addr;
    // int sender_addr_size = sizeof (sender_addr);

    // bytes_received = recvfrom(this->disco_socket, server_buf, server_buf_len, 0, (struct sockaddr *) & sender_addr, &sender_addr_size);
    // if (bytes_received < 0) {
    //     print_error("recvfrom failed with error");
    //     return 1;
    // }
    // server_buf[bytes_received] = '\0';

    return 0;
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
    builder.get_configs_to(this->manager);
    fprintf(stderr, "Finished mDNS auto discovery\n");
    fprintf(stderr, "GOT config: %s\n", json(this->manager->get_config("chroma.local")).dump(2).c_str());

    mdns_socket_close(sock);
    return 0;
}

//! NOTE: requires router to support broadcast packets
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

    this->manager->set_config(config.controllerID, config);

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

void mDNSRecordManager::get_configs_to(DiscoConfigManager *manager) {
    for (auto& record_pair : this->records) {
        auto& record = record_pair.second;
        if (record->host_name != "" && record->text.count("version") > 0) {
            fprintf(stderr, "Adding record %s\n", record->host_name.c_str());
            manager->set_config(record->host_name, {
                record->host_name,
                record->text.count("device") > 0 ? record->text["device"] : "unknown",
                stoi(record->text["version"]),
                this->ip4_table[record->host_name]
            }); // TODO: provide more info to config
        }
    }
}
