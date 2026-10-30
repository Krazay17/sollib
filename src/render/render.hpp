#pragma once

struct Vertex
{
    float pos[2];
    float color[3];
};

struct GpuCtx;

extern const char *kShader;
extern const Vertex kVertices[3];

class SolRender
{
public:
    SolRender(void *hinstance, void *hwnd, int w, int h);
    ~SolRender();
    SolRender(const SolRender &) = delete;
    SolRender &operator=(const SolRender &) = delete;

    void Resize(int w, int h);
    void Update();

    GpuCtx *ctx;

    void Buffers_Create();
    void Surface_Configure();
    void Pipeline_Create();

private:
};

int Sol_Render_Init(void *hinstance, void *hwnd);
int Sol_Render_Deinit();
int Sol_Render_Tick();
int Sol_Render_Resize(int width, int height);
