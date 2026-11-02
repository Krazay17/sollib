/*
 * File: s_renderer.cpp
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-11-01
 * 
*/
#include "render.hpp"

void SolRender::Draw(const SimState &sim, float alpha)
{
    for (int i = 0; i < sim.c.pos.size(); i++)
    {
        pos[i] = sim.c.pos_last[i] + (sim.c.pos[i] - sim.c.pos_last[i]) * alpha;
    }
}
