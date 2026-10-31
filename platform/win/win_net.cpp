#include "sol/sol.hpp"
#include <winsock2.h>
#include <WS2tcpip.h>

struct NetCtx
{
    WSADATA wsaData;
    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET clientSocket = INVALID_SOCKET;
};

static bool Socket_SetNonBlocking(SOCKET socket)
{
    u_long enabled = 1;
    return ioctlsocket(socket, FIONBIO, &enabled) != SOCKET_ERROR;
}

SolNet::SolNet()
{
    this->ctx = new NetCtx();
    int iResult;
    iResult = WSAStartup(MAKEWORD(2, 2), &this->ctx->wsaData);
    if (iResult != 0)
    {
        std::cout << "WSAStartup failed: " << std::endl;
        return;
    }
    this->socketInited = true;
}

SolNet::~SolNet()
{
    if (this->ctx)
    {
        if (this->ctx->clientSocket != INVALID_SOCKET)
            closesocket(this->ctx->clientSocket);

        if (this->ctx->listenSocket != INVALID_SOCKET)
            closesocket(this->ctx->listenSocket);

        delete this->ctx;
        this->ctx = nullptr;
    }

    if (this->socketInited)
        WSACleanup();
}

int SolNet::Server_Init()
{
    addrinfo *result = NULL, *ptr = NULL, hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int iResult;
    // Resolve the local address and port to be used by the server
    iResult = getaddrinfo(NULL, this->port, &hints, &result);
    if (iResult != 0)
    {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    this->ctx->listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (this->ctx->listenSocket == INVALID_SOCKET)
    {
        printf("Error at socket(): %d\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(this->ctx->listenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(this->ctx->listenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    if (listen(this->ctx->listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        printf("Listen failed with error: %d\n", WSAGetLastError());
        closesocket(this->ctx->listenSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Listen Socket: " << this->ctx->listenSocket << std::endl;

    if (!Socket_SetNonBlocking(this->ctx->listenSocket))
    {
        printf("ioctlsocket failed: %d\n", WSAGetLastError());
        closesocket(this->ctx->listenSocket);
        this->ctx->listenSocket = INVALID_SOCKET;
        return 1;
    }

    printf("Listening on port %s\n", this->port);

    return 0;
}

void SolNet::Server_Poll()
{
    // Try to accept a new connection.
    if (this->ctx->clientSocket == INVALID_SOCKET)
    {
        SOCKET client = accept(this->ctx->listenSocket, nullptr, nullptr);

        if (client != INVALID_SOCKET)
        {
            if (!Socket_SetNonBlocking(client))
            {
                printf("Client non-blocking setup failed: %d\n", WSAGetLastError());
                closesocket(client);
                return;
            }

            this->ctx->clientSocket = client;
            printf("Client connected!\n");
        }
        else
        {
            int error = WSAGetLastError();

            if (error != WSAEWOULDBLOCK)
                printf("accept failed: %d\n", error);
        }
    }

    // Try to receive data from the connected client.
    if (this->ctx->clientSocket != INVALID_SOCKET)
    {
        char buffer[512];

        int received = recv(
            this->ctx->clientSocket,
            buffer,
            sizeof(buffer),
            0);

        if (received > 0)
        {
            printf("Received %d bytes\n", received);

            // Process received bytes here.
        }
        else if (received == 0)
        {
            printf("Client disconnected\n");

            closesocket(this->ctx->clientSocket);
            this->ctx->clientSocket = INVALID_SOCKET;
        }
        else
        {
            int error = WSAGetLastError();

            if (error != WSAEWOULDBLOCK)
            {
                printf("recv failed: %d\n", error);

                closesocket(this->ctx->clientSocket);
                this->ctx->clientSocket = INVALID_SOCKET;
            }
        }
    }
}