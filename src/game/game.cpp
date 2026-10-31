#include "sol/sol.hpp"

void Sol_Game_Start(SolCore *core)
{
    core->net->Server_Init();
    core->Create_World();
}