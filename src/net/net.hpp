#pragma once

struct NetCtx;

class SolNet
{
public:
    SolNet();
    ~SolNet();
    int Init();
    int Server_Init();
    void Server_Poll();

    NetCtx *ctx;

private:
    const char *port = "27015";
    bool socketInited = false;
};