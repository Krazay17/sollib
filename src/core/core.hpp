/*
 * File: core.hpp
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-11-01
 *
 */
#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include "stdio.h"

#include "net/net.hpp"
#include "gfx/gfx.hpp"
#include "render/render.hpp"

struct SolWorld;

class SolCore
{
public:
    SolCore();
    ~SolCore();
    void Tick(double dt);
    void Resize(int w, int h);
    void Exit();
    void Start();
    
    double Time() const { return time; }
    double Dt() const { return dt; }
    
    bool running = true;
    
    SolNet net;
    SolGfx gfx;
    SolRender render{&gfx};
    
    std::vector<std::unique_ptr<SolWorld>> worlds;
    SolWorld *Create_World();

    const double timestep = 1.0 / 60.0;

private:
    double time = 0.0;
    double dt = 0.0;
    double accum = timestep;
};

SolCore *Sol_Init();
void Sol_Deinit(SolCore *core);