#include "mode.hpp"
#include <iostream>

void s_mode::Update(SimState &sim, double dt)
{
    std::cout << "Minion Count: " << sim.c.rules.data[0].minions << std::endl;
}