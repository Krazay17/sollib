#include "gfx.hpp"
#include "NRI.h"
#include "Extensions/NRIDeviceCreation.h"
#include "Extensions/NRISwapChain.h"

struct GpuCtx
{
    nri::CoreInterface NRI = {};
    nri::Device *m_Device = nullptr;
    nri::SwapChain *m_SwapChain = nullptr;
    nri::CommandBuffer *m_CommandBuffer = nullptr;
    nri::CommandAllocator *m_CmdAllocator = nullptr;
    nri::CommandBuffer *m_CmdBuffer = nullptr;
    nri::Fence *m_FrameFence = nullptr;
    nri::Queue *queue;
    uint64_t m_FenceValue = 0;

    // Retrieved from swapchain
    uint32_t m_BackBufferIndex = 0;
};

SolGfx::SolGfx() {}
SolGfx::~SolGfx() {}

int SolGfx::Init(void *hinstance, void *hwnd, int w, int h)
{
    ctx = new GpuCtx();

    nri::DeviceCreationDesc deviceDesc = {};
    deviceDesc.graphicsAPI = nri::GraphicsAPI::VK;
    deviceDesc.enableNRIValidation = true;

    nri::Device *device = nullptr;
    nri::Result result = nriCreateDevice(deviceDesc, device);
    if (result != nri::Result::SUCCESS || !device)
        return -1;

    ctx->m_Device = device;

    // 2. Query the Core Interface (Populates function pointers)
    result = nriGetInterface(*device, NRI_INTERFACE(nri::CoreInterface), &ctx->NRI);
    if (result != nri::Result::SUCCESS)
        return -1;

    // 3. Get the Graphics Command Queue

    // 4. Create the SwapChain bound to the window handle
    nri::SwapChainDesc swapChainDesc = {};
    swapChainDesc.window.windows.hwnd = hwnd;
    swapChainDesc.width = (uint16_t)w;
    swapChainDesc.height = (uint16_t)h;
    swapChainDesc.format = nri::SwapChainFormat::BT709_G10_16BIT;
    swapChainDesc.textureNum = 2; // Double buffering
    nri::SwapChainInterface *swap_interface = nullptr;
    result = nriGetInterface(*device, NRI_INTERFACE(nri::SwapChainInterface), &swap_interface);
    swap_interface->CreateSwapChain(*device, swapChainDesc, ctx->m_SwapChain);

    // 5. Create Command Allocator and Command Buffer for rendering
    ctx->NRI.CreateCommandAllocator(*ctx->queue, ctx->m_CmdAllocator);
    ctx->NRI.CreateCommandBuffer(*ctx->m_CmdAllocator, ctx->m_CmdBuffer);

    // 6. Create Fence for CPU-GPU synchronization
    nri::FenceDesc fence_desc = {};
    ctx->NRI.CreateFence(*device, fence_desc, ctx->m_FrameFence);

    return 0;
}
void SolGfx::Resize(int w, int h) {}

void SolGfx::Begin()
{
}
void SolGfx::End()
{
}
void SolGfx::Create_Buffers()
{
}
void SolGfx::Config_Surface()
{
}
void SolGfx::Create_Pipelines()
{
}
void SolGfx::Pipeline_TestTriangle()
{
}
void SolGfx::Pipeline_Rect()
{
}
void SolGfx::Triangle()
{
}
void SolGfx::Rectangles(const Rect *rects, uint32_t count)
{
}
