/*
 * File: xform.cpp
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-11-01
 * 
*/
#include "xform.hpp"

void s_xform::CaptureLast(SimState &sim)
{
    sim.c.pos_last = sim.c.pos;
}