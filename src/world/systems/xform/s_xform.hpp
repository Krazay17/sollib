#pragma once
#include "sol/math.hpp"

namespace SOL
{
    class XFORM
    {
        public:
        void Update(double dt);
        void UpdateDraw(float alpha);
        
        std::vector<glm::vec2> pos;
        std::vector<glm::vec2> pos_last;
        std::vector<glm::vec2> pos_draw;
    };
}
