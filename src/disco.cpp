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

#define PORT     12345
#define MAXLINE 1024

const char* LOCAL_HOST = "127.0.0.1";

void to_json(json& j, const DiscoConfig& config) {
    j = json{
        {"test", config.test}
    };
}

void from_json(const json& j, DiscoConfig& config) {
    j.at("test").get_to(config.test);
}

int write_packet(const std::vector<vec4>& pixels, char buffer[4096]) {
    // Fill packet information
    DiscoPacket packet;
    packet.start = 0; // TODO: start, end, n_frames should be calculated dynamically from pixel length
    packet.end = 150;
    packet.n_frames = 1;
    std::copy(pixels.begin(), pixels.end(), packet.data);
    int packetLen = (packet.end - packet.start) * packet.n_frames * 16 + 12;

    memcpy(buffer, &packet, packetLen);

    return packetLen;
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

DiscoConfig HTTPConfigManager::get_config() {
    return this->config;
}

DiscoConfig HTTPConfigManager::set_config(std::string id, DiscoConfig config) {
    this->config = config;
    return this->config;
}

void HTTPConfigManager::start() {
    fprintf(stderr, "Starting HTTP server\n");

    this->config_resource.disallow_all();
    this->config_resource.set_allowing("POST", true);

    this->ws.register_resource("/config", &(this->config_resource));
    this->ws.start(false);
    fprintf(stderr, "HTTP server started\n");
}

int UDPDisco::write(const std::vector<vec4>& pixels) {
#ifdef _WIN32
    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != NO_ERROR) {
        fprintf(stderr, "WSAStartup failed with error %d\n", res);
        return 1;
    }
#endif

    // Create socket
    int clientSocket = -1;
    clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (clientSocket == -1) {
        print_error("socket failed with error");
        return 1;
    }

    // Define address
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(LOCAL_HOST);

    // Write packet to socket
    char sendBuffer[4096];
    int packetLen = write_packet(pixels, sendBuffer);
    int sendResult = sendto(clientSocket, sendBuffer, packetLen, 0, (struct sockaddr *) & serverAddr, (int)sizeof(serverAddr));
    if (sendResult < 0) {
        print_error("Sending back response got an error");
        return 1;
    }

    // Get response
    int bytes_received;
    char serverBuf[1025];
    int serverBufLen = 1024;

    struct sockaddr_in SenderAddr;
    int SenderAddrSize = sizeof (SenderAddr);

    bytes_received = recvfrom(clientSocket, serverBuf, serverBufLen, 0, (struct sockaddr *) & SenderAddr, &SenderAddrSize);
    if (bytes_received < 0) {
        print_error("recvfrom failed with error");
        return 1;
    }
    serverBuf[bytes_received] = '\0';

#ifdef _WIN32
    closesocket(clientSocket);
    WSACleanup();
#else
    close(clientSocket);
#endif
    return 0;
}