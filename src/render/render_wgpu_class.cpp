/*
 * File: render_wgpu_class.cpp
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-10-29
 *
 */
#include "render.hpp"
#include <iostream>
#include <cstddef>
#include <string>
#include <string_view>

#include "webgpu/webgpu.h"

#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#include "emscripten/html5.h"
#endif

struct GpuCtx
{
    WGPUAdapter adapter;
    WGPUDevice device;
    WGPUInstance instance;
    WGPUQueue queue;
    WGPUSurface surface;
    WGPURenderPipeline pipeline;
    WGPUSurfaceConfiguration surface_conf;
    WGPUBuffer vertex_buffer;
    int width;
    int height;
    bool ready;
};

static std::string_view sv(WGPUStringView s)
{
    if (!s.data)
        return {};
    return {s.data, s.length == WGPU_STRLEN ? strlen(s.data) : s.length};
}

static void onDeviceError(WGPUDevice const *device, WGPUErrorType type, WGPUStringView message,
                          WGPU_NULLABLE void *u1, WGPU_NULLABLE void *u2)
{
    std::cout << "WGPU error (" << type << "): "
              << std::string_view(message.data, message.length) << std::endl;
}

static void adapter_info(WGPUAdapter adapter)
{
    std::cout << "Got adapter: " << adapter << std::endl;

    WGPULimits limits = {0};
    limits.nextInChain = nullptr;
    bool success = wgpuAdapterGetLimits(adapter, &limits);
    if (success)
    {
        std::cout << "Adapter limits: " << std::endl;
        std::cout << " - maxTextureDimension1D: " << limits.maxTextureDimension1D << std::endl;
        std::cout << " - maxTextureDimension2D: " << limits.maxTextureDimension2D << std::endl;
        std::cout << " - maxTextureDimension3D: " << limits.maxTextureDimension3D << std::endl;
        std::cout << " - maxTextureArrayLayers: " << limits.maxTextureArrayLayers << std::endl;
    }

    WGPUAdapterInfo info = WGPU_ADAPTER_INFO_INIT;
    wgpuAdapterGetInfo(adapter, &info);
    std::cout << "Adapter properties:" << std::endl;
    std::cout << " - vendorID: " << info.vendorID << std::endl;
    if (info.vendor.data)
    {
        std::cout << " - vendorName: " << info.vendor.data << std::endl;
    }
    if (info.architecture.data)
    {
        std::cout << " - architecture: " << info.architecture.data << std::endl;
    }
    std::cout << " - deviceID: " << info.deviceID << std::endl;
    std::cout << std::hex;
    std::cout << " - adapterType: 0x" << info.adapterType << std::endl;
    std::cout << " - backendType: 0x" << info.backendType << std::endl;
    std::cout << std::dec; // Restore decimal numbers
}

static void onDeviceRequest(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, WGPU_NULLABLE void *u1, WGPU_NULLABLE void *u2)
{
    SolRender *self = static_cast<SolRender *>(u1);
    if (status != WGPURequestDeviceStatus_Success)
    {
        std::cout << "Get Device failed: " << sv(message) << std::endl;
        return;
    }
    self->ctx->device = device;
    self->ctx->queue = wgpuDeviceGetQueue(device);

    self->Surface_Configure();
    self->Buffers_Create();
    self->Pipeline_Create();
    self->ctx->ready = true;
}

static void onAdapterRequest(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, WGPU_NULLABLE void *u1, WGPU_NULLABLE void *u2)
{
    SolRender *self = static_cast<SolRender *>(u1);
    if (status != WGPURequestAdapterStatus_Success)
    {
        std::cout << "Get Adapter failed." << sv(message) << std::endl;
        return;
    }

    self->ctx->adapter = adapter;
    adapter_info(self->ctx->adapter);

    WGPUDeviceDescriptor device_desc = WGPU_DEVICE_DESCRIPTOR_INIT;
    device_desc.uncapturedErrorCallbackInfo.callback = onDeviceError;
    WGPURequestDeviceCallbackInfo device_callback = {
        .mode = WGPUCallbackMode_AllowProcessEvents,
        .callback = onDeviceRequest,
        .userdata1 = self,
    };
    wgpuAdapterRequestDevice(self->ctx->adapter, &device_desc, device_callback);
}

SolRender::SolRender(void *hinstance, void *hwnd, int w, int h)
{
    ctx = new GpuCtx();
    this->Resize(w, h);

    const WGPUInstanceDescriptor desc = {0};
    ctx->instance = wgpuCreateInstance(&desc);
    if (!ctx->instance)
        return;
    std::cout << "WGPU instace: " << ctx->instance << std::endl;

#ifdef __EMSCRIPTEN__
    WGPUEmscriptenSurfaceSourceCanvasHTMLSelector src = {};
    src.chain.sType = WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector;
    src.selector = {"#canvas", WGPU_STRLEN};
#else
    WGPUSurfaceSourceWindowsHWND src = {};
    src.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
    src.hinstance = hinstance;
    src.hwnd = hwnd;
#endif

    WGPUSurfaceDescriptor surface_descriptor = {};
    surface_descriptor.nextInChain = &src.chain;
    ctx->surface = wgpuInstanceCreateSurface(ctx->instance, &surface_descriptor);

    WGPURequestAdapterOptions options = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
    options.compatibleSurface = ctx->surface;
    WGPURequestAdapterCallbackInfo adapter_callback = {
        .mode = WGPUCallbackMode_AllowProcessEvents,
        .callback = onAdapterRequest,
        .userdata1 = this,
    };
    wgpuInstanceRequestAdapter(ctx->instance, &options, adapter_callback);
}

SolRender::~SolRender()
{
    if (ctx->surface)
    {
        wgpuSurfaceUnconfigure(ctx->surface);
        wgpuSurfaceRelease(ctx->surface);
    }
    if (ctx->vertex_buffer)
        wgpuBufferRelease(ctx->vertex_buffer);
    if (ctx->pipeline)
        wgpuRenderPipelineRelease(ctx->pipeline);
    if (ctx->queue)
        wgpuQueueRelease(ctx->queue);
    if (ctx->device)
        wgpuDeviceRelease(ctx->device);
    if (ctx->adapter)
        wgpuAdapterRelease(ctx->adapter);
    if (ctx->instance)
        wgpuInstanceRelease(ctx->instance);

    delete ctx;
}

void SolRender::Buffers_Create()
{
    WGPUBufferDescriptor bd = WGPU_BUFFER_DESCRIPTOR_INIT;
    bd.size = sizeof(kVertices);
    bd.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
    ctx->vertex_buffer = wgpuDeviceCreateBuffer(ctx->device, &bd);

    wgpuQueueWriteBuffer(ctx->queue, ctx->vertex_buffer, 0, kVertices, sizeof(kVertices));
}

void SolRender::Surface_Configure()
{
    WGPUSurfaceCapabilities caps = WGPU_SURFACE_CAPABILITIES_INIT;
    wgpuSurfaceGetCapabilities(ctx->surface, ctx->adapter, &caps);

    ctx->surface_conf = WGPU_SURFACE_CONFIGURATION_INIT;
    ctx->surface_conf.device = ctx->device;
    ctx->surface_conf.format = caps.formats[0];
    ctx->surface_conf.width = ctx->width;
    ctx->surface_conf.height = ctx->height;
    ctx->surface_conf.usage = WGPUTextureUsage_RenderAttachment;
    ctx->surface_conf.presentMode = WGPUPresentMode_Fifo;
    ctx->surface_conf.alphaMode = caps.alphaModes[0];
    wgpuSurfaceConfigure(ctx->surface, &ctx->surface_conf);
    wgpuSurfaceCapabilitiesFreeMembers(caps);
}

void SolRender::Pipeline_Create()
{
    WGPUShaderSourceWGSL wgsl = {};
    wgsl.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgsl.code = {kShader, WGPU_STRLEN};

    WGPUShaderModuleDescriptor sm_desc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
    sm_desc.nextInChain = &wgsl.chain;
    WGPUShaderModule shader = wgpuDeviceCreateShaderModule(ctx->device, &sm_desc);

    WGPUColorTargetState color_target = WGPU_COLOR_TARGET_STATE_INIT;
    color_target.format = ctx->surface_conf.format; // stored earlier
    color_target.writeMask = WGPUColorWriteMask_All;

    WGPUFragmentState fragment = WGPU_FRAGMENT_STATE_INIT;
    fragment.module = shader;
    fragment.entryPoint = {"fs_main", WGPU_STRLEN};
    fragment.targetCount = 1;
    fragment.targets = &color_target;

    WGPUVertexAttribute attrs[2] = {};
    attrs[0].format = WGPUVertexFormat_Float32x2;
    attrs[0].offset = offsetof(Vertex, pos);
    attrs[0].shaderLocation = 0;

    attrs[1].format = WGPUVertexFormat_Float32x3;
    attrs[1].offset = offsetof(Vertex, color);
    attrs[1].shaderLocation = 1;

    WGPUVertexBufferLayout layout = WGPU_VERTEX_BUFFER_LAYOUT_INIT;
    layout.stepMode = WGPUVertexStepMode_Vertex;
    layout.arrayStride = sizeof(Vertex);
    layout.attributeCount = 2;
    layout.attributes = attrs;

    WGPURenderPipelineDescriptor pd = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
    pd.vertex.module = shader;
    pd.vertex.entryPoint = {"vs_main", WGPU_STRLEN};
    pd.fragment = &fragment;
    pd.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    pd.multisample.count = 1;
    pd.multisample.mask = ~0u;
    pd.vertex.bufferCount = 1;
    pd.vertex.buffers = &layout;

    ctx->pipeline = wgpuDeviceCreateRenderPipeline(ctx->device, &pd);
    wgpuShaderModuleRelease(shader);
}

void SolRender::Resize(int w, int h)
{
    if (w <= 0 || h <= 0)
        return;
    ctx->width = w;
    ctx->height = h;
}

static bool Sync_Size(GpuCtx *ctx)
{
    int w, h;
#ifdef __EMSCRIPTEN__
    double cw, ch;
    emscripten_get_element_css_size("#canvas", &cw, &ch);
    double dpr = emscripten_get_device_pixel_ratio();
    w = (int)(cw * dpr);
    h = (int)(ch * dpr);
    emscripten_set_canvas_element_size("#canvas", w, h);
#else
    w = ctx->width; // set by Sol_Render_Resize from the GLFW callback
    h = ctx->height;
#endif
    if (w <= 0 || h <= 0)
        return false; // minimized / mid-transition: skip the frame

    if ((uint32_t)w != ctx->surface_conf.width ||
        (uint32_t)h != ctx->surface_conf.height)
    {
        ctx->surface_conf.width = w;
        ctx->surface_conf.height = h;
        wgpuSurfaceConfigure(ctx->surface, &ctx->surface_conf);
    }
    return true;
}

void SolRender::Update()
{
    wgpuInstanceProcessEvents(ctx->instance);
    if (!ctx->ready || !Sync_Size(ctx))
        return;

    WGPUSurfaceTexture st = WGPU_SURFACE_TEXTURE_INIT;
    wgpuSurfaceGetCurrentTexture(ctx->surface, &st);

    switch (st.status)
    {
    case WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal:
        break;
    case WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal:
        if (st.texture)
            wgpuTextureRelease(st.texture);
        wgpuSurfaceConfigure(ctx->surface, &ctx->surface_conf);
        return;
    default:
        if (st.texture)
            wgpuTextureRelease(st.texture);
        wgpuSurfaceConfigure(ctx->surface, &ctx->surface_conf);
        return;
    }
    if (!st.texture)
        return;

    WGPUTextureView view = wgpuTextureCreateView(st.texture, nullptr);

    WGPUCommandEncoderDescriptor enc_desc = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(ctx->device, &enc_desc);

    WGPURenderPassColorAttachment ca = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
    ca.view = view;
    ca.loadOp = WGPULoadOp_Clear;
    ca.storeOp = WGPUStoreOp_Store;
    ca.clearValue = {0.0, 0.0, 0.0, 1.0};

    WGPURenderPassDescriptor rp_desc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
    rp_desc.colorAttachmentCount = 1;
    rp_desc.colorAttachments = &ca;

    WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &rp_desc);

    wgpuRenderPassEncoderSetPipeline(pass, ctx->pipeline);
    wgpuRenderPassEncoderSetVertexBuffer(pass, 0, ctx->vertex_buffer, 0, WGPU_WHOLE_SIZE);
    wgpuRenderPassEncoderDraw(pass, 3, 1, 0, 0);

    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);

    WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
    wgpuCommandEncoderRelease(encoder);

    wgpuQueueSubmit(ctx->queue, 1, &cmd);
    wgpuCommandBufferRelease(cmd);

#ifndef __EMSCRIPTEN__
    wgpuSurfacePresent(ctx->surface);
#endif

    wgpuTextureViewRelease(view);
    wgpuTextureRelease(st.texture);
}
