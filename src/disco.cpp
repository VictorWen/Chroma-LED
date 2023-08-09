// #include <bits/stdc++.h>
// #include <stdlib.h>
// #include <string.h>
#include <sys/types.h>
#include <string>
#include <string_view>

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
        {"test", config.test}
    };
}

void from_json(const json& j, DiscoConfig& config) {
    j.at("test").get_to(config.test);
}

void to_json(json& j, const DiscoHardwareData& config) {
    j = json{
        {"controllerID", config.controllerID},
        {"device", config.device},
        {"discoVersion", config.discoVersion},
        {"address", config.address}
    };
}

void from_json(const json& j, DiscoHardwareData& config) {
    j.at("controllerID").get_to(config.controllerID);
    j.at("device").get_to(config.device);
    j.at("discoVersion").get_to(config.discoVersion);
    if (j.count("address") > 0)
        j.at("address").get_to(config.discoVersion);
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
    fprintf(stderr, "GOT BODY: %s\n ==END== \n", json_body.data());

    json data = json::parse(json_body);
    fprintf(stderr, "GOT DATA: %s\n ==END== \n", data.dump(2).c_str());

    auto config = data.get<DiscoConfig>();
    this->manager->set_config("test-id", config);

    return std::shared_ptr<httpserver::http_response>(new httpserver::string_response("Hello, World!"));
}

DiscoConfig HTTPConfigManager::get_config(std::string id) {
    return this->configs[id];
}

DiscoConfig HTTPConfigManager::set_config(std::string id, DiscoConfig config) {
    this->configs[id] = config;
    return this->configs[id];
}

void HTTPConfigManager::add_hardware_data(DiscoHardwareData data) {
    this->hardware_data[data.controllerID] = data;
}

DiscoHardwareData HTTPConfigManager::get_hardware_data(std::string id) {
    return this->hardware_data[id];
}

void HTTPConfigManager::start() {
    fprintf(stderr, "Starting HTTP server\n");

    this->config_resource.disallow_all();
    this->config_resource.set_allowing("POST", true);

    this->ws.register_resource("/config", &(this->config_resource));
    this->ws.start(false);
    fprintf(stderr, "HTTP server started\n");
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
    DiscoHardwareData hw_data = this->manager->get_hardware_data(id);
    DiscoConfig config = this->manager->get_config(id);

    // Define address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = hw_data.address;

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
    DiscoHardwareData hardware_data = json_data.get<DiscoHardwareData>();
    hardware_data.address = sender_addr.sin_addr.s_addr;

    this->manager->add_hardware_data(hardware_data);

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