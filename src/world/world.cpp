/*
 * File: world.cpp
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-10-31
 *
 */
#include "sol/types.hpp"
#include "world.hpp"

SolWorld::SolWorld()
{
    for (auto &id : sparse)
        std::cout << id << std::endl;
}
SolWorld::~SolWorld()
{
}
void SolWorld::Tick(double dt)
{
}
void SolWorld::Step(double dt)
{
    std::cout << "World Step! " << dt << std::endl;

    xform.Update(dt);
    physics.Update(dt);
}

void SolWorld::Draw(float alpha)
{
    xform.UpdateDraw(alpha);
}

int SolWorld::Create_Ent()
{
    int id = 0;
    while (masks[id] != 0 && id < MAX_ENTS)
        id++;
    int idx = ent_count++;
    sparse[id] = idx;
    dense[idx] = id;
    masks[id] = 1;
    return id;
}
