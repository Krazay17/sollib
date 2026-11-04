/*
 * File: gfx.hpp
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-11-02
 *
 */
#pragma once

#define MAX_RECTS 0xfffu

struct Vertex
{
    float pos[2];
    float color[3];
};

struct Rect
{
    float pos[4]; // x, y, width, height
    float color[4];
};

enum Pipes
{
    PIPE_TESTTRI,
    PIPE_RECT,
    PIPE_COUNT,
};

extern const char *kShader;
extern const char *RectShader;
extern const Vertex kVertices[3];
extern const Vertex kQuad[6];

struct GpuCtx;
class SolGfx
{
public:
    SolGfx();
    ~SolGfx();

    int Init(void *hinstance, void *hwnd, int w, int h);
    void Resize(int w, int h)
    {
        if (w <= 0 || h <= 0)
            return;
        width = w;
        height = h;
    }

    void Begin();
    void End();

    void Create_Buffers();
    void Config_Surface();
    void Create_Pipelines();

    void Pipeline_TestTriangle();
    void Pipeline_Rect();

    void Triangle();
    void Rectangles(const Rect *rects, uint32_t count);

    GpuCtx *ctx = nullptr;

    int width = 800;
    int height = 600;
    bool frame_active = false;
    bool ready = false;

private:
};
