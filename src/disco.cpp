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
#else
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "disco.h"

const char* LOCAL_HOST = "127.0.0.1";

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
    std::copy(pixels.begin(), pixels.end(), packet.data);
    int packet_len = (packet.end - packet.start) * packet.n_frames * 16 + 12;

    memcpy(buffer, &packet, packet_len);

    return packet_len;
}

#ifdef _WIN32
void print_error(const char* error_msg) {
    fprintf(stderr, "%s %d\n", error_msg, WSAGetLastError());
}
#else
void print_error(const char* error_msg) {
    perror(error_msg);
}
#endif

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

int UDPDisco::write(const std::string& id, const std::vector<vec4>& pixels) {
#ifdef _WIN32
    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != NO_ERROR) {
        fprintf(stderr, "WSAStartup failed with error %d\n", res);
        return 1;
    }
#endif

    // Create socket
    // TODO: keep socket around, don't destroy after every write
    int client_socket = -1;
    client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client_socket == -1) {
        print_error("socket failed with error");
        return 1;
    }

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
    int send_result = sendto(client_socket, send_buffer, packet_len, 0, (struct sockaddr *) & server_addr, (int)sizeof(server_addr));
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

#ifdef _WIN32
    closesocket(client_socket);
    WSACleanup();
#else
    close(client_socket);
#endif
    return 0;
}

int on_mDNS_service_found(int sock, const struct sockaddr* from, size_t addrlen,
        mdns_entry_type_t entry, uint16_t query_id, uint16_t rtype,
        uint16_t rclass, uint32_t ttl, const void* data, size_t size,
        size_t name_offset, size_t name_length, size_t record_offset,
        size_t record_length, void* user_data) {
    fprintf(stderr, "GOT mDNS response, rtype: %d\n", rtype);
    const size_t buff_size = 256;
    char name_buffer[buff_size];
    
    if (entry != MDNS_ENTRYTYPE_ADDITIONAL)
        return 0;
    
    mdns_string_t entry_str = mdns_string_extract(data, size, &name_offset, name_buffer, buff_size);
    std::string entry_name(entry_str.str, entry_str.length - 1); // subtract 1 to remove period

    auto manager = static_cast<DiscoConfigManager*>(user_data);

    if (rtype == MDNS_RECORDTYPE_PTR) {
        mdns_string_t name_str = mdns_record_parse_ptr(data, size, record_offset, record_length,
            name_buffer, buff_size);
        std::string name(name_str.str, name_str.length);
        fprintf(stderr, "GOT PTR record: %s\n", name.c_str());
    }
    else if (rtype == MDNS_RECORDTYPE_SRV) {
        mdns_record_srv_t srv = mdns_record_parse_srv(data, size, record_offset, record_length,
            name_buffer, buff_size);
        std::string name(srv.name.str, srv.name.length);
        fprintf(stderr, "GOT SRV record: %s, port: %d\n", name.c_str(), srv.port);
    }
    else if (rtype == MDNS_RECORDTYPE_A) {
        struct sockaddr_in addr;
		mdns_record_parse_a(data, size, record_offset, record_length, &addr);
        inet_ntop(AF_INET, &(addr.sin_addr), name_buffer, buff_size);
        std::string name(name_buffer);
        fprintf(stderr, "GOT A record: %s %s\n", entry_name.c_str(), name.c_str());
        manager->set_config(entry_name, {
            entry_name,
            "esp8266",
            1,
            name
        });
    }

    return 0;
}

int DiscoDiscoverer::send_mDNS_query() {
#ifdef _WIN32
    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != NO_ERROR) {
        fprintf(stderr, "WSAStartup failed with error %d\n", res);
        return 1;
    }
#endif
    // Create socket
    int sock = -1;
    sock = mdns_socket_open_ipv4(NULL);
    if (sock < 0) {
        print_error("socket failed with error");
        return 1;
    }

    const size_t buff_size = 2048;
    void* buffer[buff_size];
    if (mdns_query_send(sock, MDNS_RECORDTYPE_PTR, "_discoConnect._tcp.local", 24, buffer, buff_size, 0) < 0) {
        print_error("mdns query failed");
        return 1;
    }

    // Collect responses
    const double timeout = 2;
    auto timer = std::chrono::high_resolution_clock::now();
    while (std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - timer).count() < timeout * 1e3) {
        int results = mdns_query_recv(sock, buffer, buff_size, on_mDNS_service_found, this->manager, 0);
        if (results > 0)
            timer = std::chrono::high_resolution_clock::now();
    }

#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
    return 0;
}

//! NOTE: requires router to support broadcast packets
int DiscoDiscoverer::send_broadcast() { // TODO: add support for multicast
#ifdef _WIN32
    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != NO_ERROR) {
        fprintf(stderr, "WSAStartup failed with error %d\n", res);
        return 1;
    }
#endif

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
    WSACleanup();
#else
    close(client_socket);
#endif
    return 0;
}