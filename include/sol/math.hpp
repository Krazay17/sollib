#pragma once

#include "sol/base.hpp"
#include "glm/glm.hpp"

namespace SOL
{
    union V3
    {
        struct
        {
            float x;
            float y;
            float z;
        };
        float raw[3];
    };
    union V2
    {
        struct
        {
            float x;
            float y;
        };
        float raw[2];
    };
}