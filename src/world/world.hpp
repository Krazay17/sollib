#pragma once
#include "sol/math.hpp"

#include "systems/xform/s_xform.hpp"
#include "systems/physics/s_physics.hpp"

#define MAX_ENTS 0xffffu

class SolWorld
{
public:
    SolWorld();
    ~SolWorld();

    void Tick(double dt);
    void Step(double dt);
    void Draw(float alpha);
    int Create_Ent();

    std::array<int, MAX_ENTS> sparse;
    std::array<int, MAX_ENTS> dense;
    std::array<int, MAX_ENTS> masks;
    int ent_count = 0;

    SOL::XFORM xform;
    SOL::PHYSICS physics;
};