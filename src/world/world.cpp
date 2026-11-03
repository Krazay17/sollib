/*
 * File: world.cpp
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-10-31
 *
 */
#include "world.hpp"
#include <iostream>

#include "sim/xform/xform.hpp"
#include "sim/physics/physics.hpp"
#include "sim/mode/mode.hpp"

void SolWorld::Tick(double dt)
{
}
void SolWorld::Step(double dt)
{
    s_xform::CaptureLast(sim_state);
    s_physics::Step(sim_state, dt);
    s_mode::Update(sim_state, dt);
}

Entity SolWorld::Create_Ent()
{
    uint32_t id;
    if(!sim_state.free_ents.empty())
    {
        id = sim_state.free_ents.back();
        sim_state.free_ents.pop_back();
    }
    else
    {
        id = static_cast<uint32_t>(sim_state.ents.size());
        sim_state.ents.emplace_back();
    }
    EntityData &data = sim_state.ents[id];
    data.alive = true;
    data.mask = 0;

    return {id, data.gen};
}
