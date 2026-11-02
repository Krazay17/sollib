/*
 * File: sol_core.cpp
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-10-30
 *
 */
#include "core.hpp"
#include "world/world.hpp"
#include "gfx/gfx.hpp"

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
}

void SolCore::Exit()
{
    running = false;
}

void SolCore::Tick(double dt)
{
    this->dt = dt;
    accum += dt;
    while (accum >= timestep)
    {
        accum -= timestep;
        for (auto &world : worlds)
        {
            world->Step(timestep);
        }
    }
    float alpha = static_cast<float>(accum / timestep);
    for (auto &world : worlds)
        world->Draw(alpha);

    gfx.Update();
    std::cout << "Delta Time: " << dt << std::endl;
}

void SolCore::Resize(int w, int h)
{
    gfx.Resize(w, h);
}

SolWorld *SolCore::Create_World()
{
    auto world = std::make_unique<SolWorld>();
    SolWorld *ptr = world.get();
    worlds.push_back(std::move(world));

    return ptr;
}
