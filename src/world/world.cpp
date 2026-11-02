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
    // std::cout << "World Step! " << dt << std::endl;

    s_xform::CaptureLast(sim_state);
    s_physics::Step(sim_state, dt);
    s_mode::Update(sim_state, dt);
}

void SolWorld::Draw(float alpha)
{
}

int SolWorld::Create_Ent()
{
    int id = 0;
    while (id < MAX_ENTS && sim_state.ents.masks[id] != 0)
        id++;
    int idx = sim_state.ents.count++;
    sim_state.ents.sparse[id] = idx;
    sim_state.ents.dense[idx] = id;
    sim_state.ents.masks[id] = 1;
    return id;
}
