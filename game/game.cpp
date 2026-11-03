/*
 * File: game.cpp
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-11-01
 *
 */
#include "game.hpp"
#include "world/world.hpp"

void SolCore::Start()
{
    net.Server_Init();
    SolWorld *world = Create_World();

    // int id = world->Create_Ent();
    // world->sim_state.c.pos[0] = glm::vec3(0.5, 1.0, 0);
    // world->sim_state.c.view[0] = VIEW_TESTRECT;
    // world->sim_state.c.body.dims[0] = glm::vec3(0.5, 0.5, 0);
    // world->sim_state.ents.masks[0] = 1u << Masks::HAS_VIEW;
}
