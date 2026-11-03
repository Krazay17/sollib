/*
 * File: render.hpp
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-11-02
 * 
*/
#pragma once

#include "sim/sim_state.hpp"
#include "gfx/gfx.hpp"

class SolRender
{
public:
    SolRender(SolGfx *gfx) { this->gfx = gfx; };
    void Update(const SimState &sim, float alpha);
    void Draw(const SimState &sim);

private:
    SolGfx *gfx;
    std::array<glm::vec3, MAX_ENTS> pos;
    std::vector<Rect> rects;
};
