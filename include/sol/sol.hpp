#pragma once

#include "sol/types.hpp"

struct SolWorld;
struct SolRender;

struct NetCtx;
class SolNet
{
public:
    SolNet();
    ~SolNet();

    int Server_Init();
    void Server_Poll();

    NetCtx *ctx;

private:
    const char *port = "27015";
    bool socketInited = false;
};

class SolCore
{
public:
    SolCore();
    ~SolCore();
    void Tick(double dt);
    void Render_Init(void *hinstance, void *hwnd, int w, int h);
    void Net_Init();
    void Resize(int w, int h);
    void Exit();
    void Create_World();

    double Time() const { return time; }
    double Dt() const { return time; }

    bool running = true;

    SolNet *net = nullptr;
    SolRender *renderer = nullptr;
    std::vector<SolWorld *> worlds;

    const double timestep = 1.0 / 60.0;

private:
    double time = 0.0;
    double dt = 0.0;
    double accum = timestep;
};

void Sol_Game_Start(SolCore *core);
