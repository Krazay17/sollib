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

    int id = world->Create_Ent();
    world->sim_state.c.pos.push_back(glm::vec3(0,0,0));
}
