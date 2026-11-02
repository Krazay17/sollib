#pragma once

#include "sim/sim_state.hpp"
#include "gfx/gfx.hpp"

class SolRender
{
public:
    SolRender(SolGfx *gfx) { this->gfx = gfx; };
    void Draw(const SimState &, float alpha);

private:
    SolGfx *gfx;
    std::vector<glm::vec3> pos;
};
