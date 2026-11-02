#include "net/net.hpp"

#include "core/core.hpp"
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
    
}
SolNet::~SolNet()
{
    if (ctx)
    {
        if (ctx->clientSocket != INVALID_SOCKET)
            closesocket(ctx->clientSocket);

        if (ctx->listenSocket != INVALID_SOCKET)
            closesocket(ctx->listenSocket);

        delete ctx;
        ctx = nullptr;
    }

    if (socketInited)
        WSACleanup();
}

int SolNet::Init()
{
    ctx = new NetCtx();
    int iResult;
    iResult = WSAStartup(MAKEWORD(2, 2), &ctx->wsaData);
    if (iResult != 0)
    {
        std::cout << "WSAStartup failed: " << std::endl;
        return 1;
    }
    socketInited = true;
    
    return 0;
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
    iResult = getaddrinfo(NULL, port, &hints, &result);
    if (iResult != 0)
    {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    ctx->listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (ctx->listenSocket == INVALID_SOCKET)
    {
        printf("Error at socket(): %d\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ctx->listenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ctx->listenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    if (listen(ctx->listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        printf("Listen failed with error: %d\n", WSAGetLastError());
        closesocket(ctx->listenSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Listen Socket: " << ctx->listenSocket << std::endl;

    if (!Socket_SetNonBlocking(ctx->listenSocket))
    {
        printf("ioctlsocket failed: %d\n", WSAGetLastError());
        closesocket(ctx->listenSocket);
        ctx->listenSocket = INVALID_SOCKET;
        return 1;
    }

    printf("Listening on port %s\n", port);

    return 0;
}

void SolNet::Server_Poll()
{
    // Try to accept a new connection.
    if (ctx->clientSocket == INVALID_SOCKET)
    {
        SOCKET client = accept(ctx->listenSocket, nullptr, nullptr);

        if (client != INVALID_SOCKET)
        {
            if (!Socket_SetNonBlocking(client))
            {
                printf("Client non-blocking setup failed: %d\n", WSAGetLastError());
                closesocket(client);
                return;
            }

            ctx->clientSocket = client;
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
    if (ctx->clientSocket != INVALID_SOCKET)
    {
        char buffer[512];

        int received = recv(
            ctx->clientSocket,
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

            closesocket(ctx->clientSocket);
            ctx->clientSocket = INVALID_SOCKET;
        }
        else
        {
            int error = WSAGetLastError();

            if (error != WSAEWOULDBLOCK)
            {
                printf("recv failed: %d\n", error);

                closesocket(ctx->clientSocket);
                ctx->clientSocket = INVALID_SOCKET;
            }
        }
    }
}