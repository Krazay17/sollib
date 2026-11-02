/*
 * File: sim_state.hpp
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-11-01
 *
 */
#pragma once

#include <array>
#include "core/math.hpp"

#define MAX_ENTS 0xffu

struct Entities
{
    std::array<uint16_t, MAX_ENTS> sparse, dense;
    std::array<uint64_t, MAX_ENTS> masks;
    std::array<uint8_t, MAX_ENTS> generation;
    int count = 0;
};

struct Components
{
    std::vector<glm::vec3> pos, pos_last, vel, scale;
    std::vector<uint8_t> shape;
};

struct Singletons
{
    struct GameMode
    {
        uint16_t kind = 0;
    } mode;
};

struct SimState
{
    uint32_t tick = 0;
    Entities ents;
    Components c;
    Singletons s;
};