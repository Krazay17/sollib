/*
 * File: world.hpp
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-11-01
 * 
*/
#pragma once

#include "sim/sim_state.hpp"

class SolWorld
{
public:
    void Tick(double dt);
    void Step(double dt);
    Entity Create_Ent();

    SimState sim_state;
};
