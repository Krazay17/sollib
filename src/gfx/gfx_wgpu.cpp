/*
 * File: render_wgpu_class.cpp
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-10-29
 *
 */
#include "gfx.hpp"
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

    std::array<WGPURenderPipeline, PIPE_COUNT> pipelines;
    WGPUBuffer vertex_buffer;
    WGPUBuffer quad_buffer;
    WGPUBuffer rect_buffer;

    WGPUTexture frame_texture;
    WGPUTextureView frame_view;

    WGPUCommandEncoder encoder;
    WGPURenderPassEncoder pass;

    WGPUSurfaceConfiguration surface_conf;

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
    SolGfx *self = static_cast<SolGfx *>(u1);
    if (status != WGPURequestDeviceStatus_Success)
    {
        std::cout << "Get Device failed: " << sv(message) << std::endl;
        return;
    }
    self->ctx->device = device;
    self->ctx->queue = wgpuDeviceGetQueue(device);

    self->Config_Surface();
    self->Create_Buffers();
    self->Create_Pipelines();
    self->ready = true;
}

static void onAdapterRequest(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, WGPU_NULLABLE void *u1, WGPU_NULLABLE void *u2)
{
    SolGfx *self = static_cast<SolGfx *>(u1);
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

SolGfx::SolGfx()
{
    ctx = new GpuCtx();
    *ctx = {};
}

SolGfx::~SolGfx()
{
    if (ctx->surface)
    {
        wgpuSurfaceUnconfigure(ctx->surface);
        wgpuSurfaceRelease(ctx->surface);
    }
    if (ctx->vertex_buffer)
        wgpuBufferRelease(ctx->vertex_buffer);
    if (ctx->quad_buffer)
        wgpuBufferRelease(ctx->quad_buffer);
    if (ctx->rect_buffer)
        wgpuBufferRelease(ctx->rect_buffer);
    if (ctx->pipelines.size() > 0)
    {
        for (auto &p : ctx->pipelines)
            wgpuRenderPipelineRelease(p);
    }
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

int SolGfx::Init(void *hinstance, void *hwnd, int w, int h)
{
    Resize(w, h);

    const WGPUInstanceDescriptor desc = {0};
    ctx->instance = wgpuCreateInstance(&desc);
    if (!ctx->instance)
        return 1;
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

    return 0;
}

void SolGfx::Create_Buffers()
{
    WGPUBufferDescriptor bd = WGPU_BUFFER_DESCRIPTOR_INIT;
    bd.size = sizeof(kVertices);
    bd.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
    ctx->vertex_buffer = wgpuDeviceCreateBuffer(ctx->device, &bd);

    wgpuQueueWriteBuffer(ctx->queue, ctx->vertex_buffer, 0, kVertices, sizeof(kVertices));

    WGPUBufferDescriptor quad_desc = WGPU_BUFFER_DESCRIPTOR_INIT;
    quad_desc.size = sizeof(kQuad);
    quad_desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
    ctx->quad_buffer = wgpuDeviceCreateBuffer(ctx->device, &quad_desc);

    wgpuQueueWriteBuffer(ctx->queue, ctx->quad_buffer, 0, kQuad, sizeof(kQuad));

    WGPUBufferDescriptor rect_desc = WGPU_BUFFER_DESCRIPTOR_INIT;
    rect_desc.size = sizeof(Rect) * MAX_RECTS;
    rect_desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
    ctx->rect_buffer = wgpuDeviceCreateBuffer(ctx->device, &rect_desc);
}

void SolGfx::Config_Surface()
{
    WGPUSurfaceCapabilities caps = WGPU_SURFACE_CAPABILITIES_INIT;
    wgpuSurfaceGetCapabilities(ctx->surface, ctx->adapter, &caps);

    ctx->surface_conf = WGPU_SURFACE_CONFIGURATION_INIT;
    ctx->surface_conf.device = ctx->device;
    ctx->surface_conf.format = caps.formats[0];
    ctx->surface_conf.width = width;
    ctx->surface_conf.height = height;
    ctx->surface_conf.usage = WGPUTextureUsage_RenderAttachment;
    ctx->surface_conf.presentMode = WGPUPresentMode_Fifo;
    ctx->surface_conf.alphaMode = caps.alphaModes[0];
    wgpuSurfaceConfigure(ctx->surface, &ctx->surface_conf);
    wgpuSurfaceCapabilitiesFreeMembers(caps);
}

void SolGfx::Pipeline_TestTriangle()
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

    ctx->pipelines[PIPE_TESTTRI] = wgpuDeviceCreateRenderPipeline(ctx->device, &pd);
    wgpuShaderModuleRelease(shader);
}

void SolGfx::Pipeline_Rect()
{
    WGPUShaderSourceWGSL wgsl = {};
    wgsl.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgsl.code = {RectShader, WGPU_STRLEN};

    WGPUShaderModuleDescriptor sm_desc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
    sm_desc.nextInChain = &wgsl.chain;
    WGPUShaderModule shader = wgpuDeviceCreateShaderModule(ctx->device, &sm_desc);

    WGPUColorTargetState color_target = WGPU_COLOR_TARGET_STATE_INIT;
    color_target.format = ctx->surface_conf.format; // stored earlier
    color_target.writeMask = WGPUColorWriteMask_All;

    WGPUVertexAttribute quad_attrs[1] = {};
    quad_attrs[0].format = WGPUVertexFormat_Float32x2;
    quad_attrs[0].offset = 0;
    quad_attrs[0].shaderLocation = 0;

    WGPUVertexAttribute rect_attrs[2] = {};
    rect_attrs[0].format = WGPUVertexFormat_Float32x4;
    rect_attrs[0].offset = offsetof(Rect, pos);
    rect_attrs[0].shaderLocation = 1;

    rect_attrs[1].format = WGPUVertexFormat_Float32x4;
    rect_attrs[1].offset = offsetof(Rect, color);
    rect_attrs[1].shaderLocation = 2;

    WGPUVertexBufferLayout layouts[2] = {};
    layouts[0].stepMode = WGPUVertexStepMode_Vertex;
    layouts[0].arrayStride = sizeof(Vertex);
    layouts[0].attributeCount = 1;
    layouts[0].attributes = quad_attrs;

    layouts[1].stepMode = WGPUVertexStepMode_Instance;
    layouts[1].arrayStride = sizeof(Rect);
    layouts[1].attributeCount = 2;
    layouts[1].attributes = rect_attrs;

    WGPUFragmentState fragment = WGPU_FRAGMENT_STATE_INIT;
    fragment.module = shader;
    fragment.entryPoint = {"fs_main", WGPU_STRLEN};
    fragment.targetCount = 1;
    fragment.targets = &color_target;

    WGPURenderPipelineDescriptor pd = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
    pd.vertex.module = shader;
    pd.vertex.entryPoint = {"vs_main", WGPU_STRLEN};
    pd.fragment = &fragment;
    pd.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    pd.multisample.count = 1;
    pd.multisample.mask = ~0u;
    pd.vertex.bufferCount = 2;
    pd.vertex.buffers = layouts;

    ctx->pipelines[PIPE_RECT] = wgpuDeviceCreateRenderPipeline(ctx->device, &pd);
    wgpuShaderModuleRelease(shader);
}

void SolGfx::Create_Pipelines()
{
    Pipeline_TestTriangle();
    Pipeline_Rect();
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
    w = ctx->width; // set by Sol_Gfx_Resize from the GLFW callback
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

void SolGfx::Begin()
{
    if (frame_active)
        return;

    wgpuInstanceProcessEvents(ctx->instance);

    if (!ready || !Sync_Size(ctx))
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

    ctx->frame_texture = st.texture;
    ctx->frame_view = wgpuTextureCreateView(ctx->frame_texture, nullptr);

    WGPUCommandEncoderDescriptor enc_desc = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
    ctx->encoder = wgpuDeviceCreateCommandEncoder(ctx->device, &enc_desc);

    WGPURenderPassColorAttachment ca = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
    ca.view = ctx->frame_view;
    ca.loadOp = WGPULoadOp_Clear;
    ca.storeOp = WGPUStoreOp_Store;
    ca.clearValue = {0.0, 0.0, 0.0, 1.0};

    WGPURenderPassDescriptor rp_desc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
    rp_desc.colorAttachmentCount = 1;
    rp_desc.colorAttachments = &ca;

    ctx->pass = wgpuCommandEncoderBeginRenderPass(ctx->encoder, &rp_desc);

    frame_active = true;
}

void SolGfx::End()
{
    if (!frame_active)
        return;
    wgpuRenderPassEncoderEnd(ctx->pass);
    wgpuRenderPassEncoderRelease(ctx->pass);
    ctx->pass = nullptr;

    WGPUCommandBufferDescriptor cmd_desc = {};
    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(ctx->encoder, &cmd_desc);
    wgpuCommandEncoderRelease(ctx->encoder);
    ctx->encoder = nullptr;

    wgpuQueueSubmit(ctx->queue, 1, &cmd);
    wgpuCommandBufferRelease(cmd);

#ifndef __EMSCRIPTEN__
    wgpuSurfacePresent(ctx->surface);
#endif

    wgpuTextureViewRelease(ctx->frame_view);
    wgpuTextureRelease(ctx->frame_texture);

    ctx->frame_view = nullptr;
    ctx->frame_texture = nullptr;
    frame_active = false;
}

void SolGfx::Triangle()
{
    if (!frame_active)
        return;
    wgpuRenderPassEncoderSetPipeline(ctx->pass, ctx->pipelines[PIPE_TESTTRI]);
    wgpuRenderPassEncoderSetVertexBuffer(ctx->pass, 0, ctx->vertex_buffer, 0, WGPU_WHOLE_SIZE);
    wgpuRenderPassEncoderDraw(ctx->pass, 3, 1, 0, 0);
}

void SolGfx::Rectangles(const Rect *rects, uint32_t count)
{
    if (!frame_active || !rects || count == 0)
        return;

    if (count > MAX_RECTS)
        count = MAX_RECTS;

    wgpuQueueWriteBuffer(
        ctx->queue,
        ctx->rect_buffer,
        0,
        rects,
        sizeof(Rect) * count);

    wgpuRenderPassEncoderSetPipeline(ctx->pass, ctx->pipelines[PIPE_RECT]);
    wgpuRenderPassEncoderSetVertexBuffer(ctx->pass, 0, ctx->quad_buffer, 0, WGPU_WHOLE_SIZE);
    wgpuRenderPassEncoderSetVertexBuffer(ctx->pass, 1, ctx->rect_buffer, 0, WGPU_WHOLE_SIZE);
    wgpuRenderPassEncoderDraw(ctx->pass, 6, count, 0, 0);
}
