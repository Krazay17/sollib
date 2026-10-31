/*
 * File: sol_core.cpp
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-10-30
 *
 */
#include "sol/sol.hpp"
#include "world/world.hpp"
#include "render/render.hpp"

SolCore *Sol_Init()
{
    return new SolCore();
}

void Sol_Deinit(SolCore *core)
{
    delete core;
}

SolCore::SolCore()
{
}

SolCore::~SolCore()
{
    if (renderer)
    {
        delete renderer;
        renderer = nullptr;
    }
    if (net)
    {
        delete net;
        net = nullptr;
    }
}
void SolCore::Exit()
{
    this->running = false;
}

void SolCore::Tick(double dt)
{
    this->dt = dt;
    accum += dt;
    while (accum >= timestep)
    {
        accum -= timestep;
        for (auto &world : worlds)
            world->Step(timestep);
    }
    float alpha = static_cast<float>(accum / timestep);
    for (auto &world : worlds)
        world->Draw(alpha);

    renderer->Update();
    std::cout << "Delta Time: " << dt << std::endl;
}

void SolCore::Render_Init(void *hinstance, void *hwnd, int w, int h)
{
    renderer = new SolRender(hinstance, hwnd, w, h);
}

void SolCore::Net_Init()
{
    net = new SolNet();
}

void SolCore::Resize(int w, int h)
{
    renderer->Resize(w, h);
}

void SolCore::Create_World()
{
    worlds.push_back(new SolWorld);
}
