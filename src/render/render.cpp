/*
 * File: s_renderer.cpp
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-11-01
 *
 */
#include "render.hpp"
static float y = 0.5f;
void SolRender::Update(const SimState &sim, float alpha)
{
     rects.clear();
    Rect test_rect = {{0.5, alpha, 0.5, 0.5}, {1, 1, 1, 1}};
    rects.push_back(test_rect);
    y += -0.001f;

    // for (auto &e : sim.ents)
    // {
    //     pos[e.id] = sim.c.xforms.data[e.id].pos_last + (sim.c.xforms.data[e.id].pos - sim.c.xforms.data[e.id].pos_last) * alpha;
    // }
    // for (auto &e : sim.ents)
    // {
    //     if (!(e.mask & 1u << Masks::HAS_VIEW))
    //         continue;
    //     glm::vec3 dims = sim.c.body.dims[e.id];
    //     switch (sim.c.view[e.id])
    //     {
    //     case VIEW_TESTRECT:
    //         Rect test_rect = {{pos[e.id].x, pos[e.id].y, dims.x, dims.y}, {1, 1, 1, 1}};
    //         rects.push_back(test_rect);
    //         break;
    //     }
    // }
}

void SolRender::Draw(const SimState &sim)
{
    gfx->Rectangles(rects.data(), rects.size());
    gfx->Triangle();
}
