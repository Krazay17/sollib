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

    gfx.Begin();
    printf("dt: %.4f alpha: %.4f\n", dt, alpha);
    render.Update(worlds[view_world]->sim_state, alpha);
    render.Draw(worlds[view_world]->sim_state);
    gfx.End();
}


SolWorld *SolCore::Create_World()
{
    auto world = std::make_unique<SolWorld>();
    SolWorld *ptr = world.get();
    worlds.push_back(std::move(world));

    return ptr;
}
