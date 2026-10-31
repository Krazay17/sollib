#include "s_xform.hpp"

void SOL::XFORM::Update(double dt)
{
    pos_last = pos;
}
void SOL::XFORM::UpdateDraw(float alpha)
{
    for(int i =0; i < pos.size();i++)
    {
        pos_draw[i] = pos_last[i] + (pos[i] = pos_last[i]) * alpha;
    }
}
