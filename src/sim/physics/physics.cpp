#include "physics.hpp"
#include <iostream>

void s_physics::Step(SimState &sim, double dt)
{
    glm::vec3 grav = {0, -9.81, 0};
    for (auto &p : sim.c.pos)
    {
        p += grav * (float)dt;
    }
    std::cout << sim.c.pos[0].y << std::endl;
}
