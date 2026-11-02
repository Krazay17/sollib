#pragma once

struct Vertex
{
    float pos[2];
    float color[3];
};

struct GpuCtx;

extern const char *kShader;
extern const Vertex kVertices[3];

class SolGfx
{
public:
    ~SolGfx();

    int Init(void *hinstance, void *hwnd, int w, int h);
    void Resize(int w, int h);
    void Update();

    GpuCtx *ctx;

    void Buffers_Create();
    void Surface_Configure();
    void Pipeline_Create();

private:
};

int Sol_Gfx_Init(void *hinstance, void *hwnd);
int Sol_Gfx_Deinit();
int Sol_Gfx_Tick();
int Sol_Gfx_Resize(int width, int height);
