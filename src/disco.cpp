// #include <bits/stdc++.h>
// #include <stdlib.h>
// #include <string.h>
#include <sys/types.h>

#ifdef _WIN32
#include <WinSock2.h>
#include <Ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif


#include "disco.h"

#define PORT     8080
#define MAXLINE 1024

int UDPDisco::write(const std::vector<vec4>& pixels) {
#ifdef _WIN32
    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != NO_ERROR) {
        fprintf(stderr, "WSAStartup failed with error %d\n", res);
        return 1;
    }

    // Initalize to default value to be safe.
    SOCKET clientSocket = INVALID_SOCKET;
    clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (clientSocket == INVALID_SOCKET) {
        fprintf(stderr, "socket failed with error %d\n", WSAGetLastError());
        return 1;
    }

    struct sockaddr_in serverAddr;
    const char* local_host = "127.0.0.1";
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(local_host);
    
    
    DiscoPacket packet;
    packet.start = 0;
    packet.end = 150;
    packet.n_frames = 1;
    std::copy(pixels.begin(), pixels.end(), packet.data);
    int packetLen = (packet.end - packet.start) * packet.n_frames * 16 + 12;

    char sendBuffer[4096];
    memcpy(sendBuffer, &packet, packetLen);
    
    int sendResult = sendto(clientSocket, sendBuffer, packetLen, 0, (SOCKADDR *) & serverAddr, (int)sizeof(serverAddr));
    if (sendResult == SOCKET_ERROR) {
        fprintf(stderr, "Sending back response got an error: %d\n", WSAGetLastError());
        return 1;
    }

    int bytes_received;
    char serverBuf[1025];
    int serverBufLen = 1024;

    // Keep a separate address struct to store sender information. 
    struct sockaddr_in SenderAddr;
    int SenderAddrSize = sizeof (SenderAddr);

    // fprintf(stderr, "Receiving datagrams on %s\n", "127.0.0.1");
    bytes_received = recvfrom(clientSocket, serverBuf, serverBufLen, 0, (SOCKADDR *) & SenderAddr, &SenderAddrSize);
    if (bytes_received == SOCKET_ERROR) {
        fprintf(stderr, "recvfrom failed with error %d\n", WSAGetLastError());
        return 1;
    }
    serverBuf[bytes_received] = '\0';
    // fprintf(stderr, "got back: %s\n", serverBuf);

    closesocket(clientSocket);
    WSACleanup();
#endif
    return 0;
}