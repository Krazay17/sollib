#include "render.hpp"
#include <iostream>
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

GpuCtx user_data = {0};

SolRender::SolRender(void *hinstance, void *hwnd)
{
    ctx = new GpuCtx();

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
        .userdata1 = ctx,
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
    if (ctx->queue)
        wgpuQueueRelease(ctx->queue);
    if (ctx->device)
        wgpuDeviceRelease(ctx->device);
    if (ctx->adapter)
        wgpuAdapterRelease(ctx->adapter);
    if (ctx->instance)
        wgpuInstanceRelease(ctx->instance);
}

static std::string_view sv(WGPUStringView s)
{
    if (!s.data)
        return {};
    return {s.data, s.length == WGPU_STRLEN ? strlen(s.data) : s.length};
}

static void Buffers_Create()
{
    WGPUBufferDescriptor bd = WGPU_BUFFER_DESCRIPTOR_INIT;
    bd.size = sizeof(kVertices);
    bd.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
    user_data.vertex_buffer = wgpuDeviceCreateBuffer(user_data.device, &bd);

    wgpuQueueWriteBuffer(user_data.queue, user_data.vertex_buffer, 0, kVertices, sizeof(kVertices));
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

static void Surface_Configure()
{
    WGPUSurfaceCapabilities caps = WGPU_SURFACE_CAPABILITIES_INIT;
    wgpuSurfaceGetCapabilities(user_data.surface, user_data.adapter, &caps);

    user_data.surface_conf = WGPU_SURFACE_CONFIGURATION_INIT;
    user_data.surface_conf.device = user_data.device;
    user_data.surface_conf.format = caps.formats[0];
    user_data.surface_conf.width = user_data.width;
    user_data.surface_conf.height = user_data.height;
    user_data.surface_conf.usage = WGPUTextureUsage_RenderAttachment;
    user_data.surface_conf.presentMode = WGPUPresentMode_Fifo;
    user_data.surface_conf.alphaMode = caps.alphaModes[0];
    wgpuSurfaceConfigure(user_data.surface, &user_data.surface_conf);
    wgpuSurfaceCapabilitiesFreeMembers(caps);
}

static void Pipeline_Create()
{
    WGPUShaderSourceWGSL wgsl = {};
    wgsl.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgsl.code = {kShader, WGPU_STRLEN};

    WGPUShaderModuleDescriptor sm_desc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
    sm_desc.nextInChain = &wgsl.chain;
    WGPUShaderModule shader = wgpuDeviceCreateShaderModule(user_data.device, &sm_desc);

    WGPUColorTargetState color_target = WGPU_COLOR_TARGET_STATE_INIT;
    color_target.format = user_data.surface_conf.format; // stored earlier
    color_target.writeMask = WGPUColorWriteMask_All;

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

    user_data.pipeline = wgpuDeviceCreateRenderPipeline(user_data.device, &pd);
    wgpuShaderModuleRelease(shader);
}

static void onDeviceRequest(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, WGPU_NULLABLE void *u1, WGPU_NULLABLE void *u2)
{
    GpuCtx *ctx = (GpuCtx *)u1;
    if (status != WGPURequestDeviceStatus_Success)
    {
        std::cout << "Get Device failed: " << sv(message) << std::endl;
        return;
    }
    ctx->device = device;
    ctx->queue = wgpuDeviceGetQueue(device);

    Surface_Configure();
    Pipeline_Create();
    ctx->ready = true;
}

static void onAdapterRequest(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, WGPU_NULLABLE void *u1, WGPU_NULLABLE void *u2)
{
    GpuCtx *ctx = (GpuCtx *)u1;
    if (status != WGPURequestAdapterStatus_Success)
    {
        std::cout << "Get Adapter failed." << sv(message) << std::endl;
        return;
    }

    ctx->adapter = adapter;
    adapter_info(user_data.adapter);

    WGPUDeviceDescriptor device_desc = WGPU_DEVICE_DESCRIPTOR_INIT;
    device_desc.uncapturedErrorCallbackInfo.callback = onDeviceError;
    WGPURequestDeviceCallbackInfo device_callback = {
        .mode = WGPUCallbackMode_AllowProcessEvents,
        .callback = onDeviceRequest,
        .userdata1 = ctx,
    };
    wgpuAdapterRequestDevice(ctx->adapter, &device_desc, device_callback);
}

int Sol_Render_Init(void *hinstance, void *hwnd)
{
    const WGPUInstanceDescriptor desc = {0};
    user_data.instance = wgpuCreateInstance(&desc);
    if (!user_data.instance)
        return 1;
    std::cout << "WGPU instace: " << user_data.instance << std::endl;

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
    user_data.surface = wgpuInstanceCreateSurface(user_data.instance, &surface_descriptor);

    WGPURequestAdapterOptions options = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
    options.compatibleSurface = user_data.surface;
    WGPURequestAdapterCallbackInfo adapter_callback = {.mode = WGPUCallbackMode_AllowProcessEvents, .callback = onAdapterRequest, .userdata1 = &user_data};
    wgpuInstanceRequestAdapter(user_data.instance, &options, adapter_callback);

    return 0;
}

int Sol_Render_Resize(int w, int h)
{
    if (w <= 0 || h <= 0)
        return 1;
    user_data.width = w;
    user_data.height = h;
    return 0;
}

static bool Sync_Size()
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
    w = user_data.width; // set by Sol_Render_Resize from the GLFW callback
    h = user_data.height;
#endif
    if (w <= 0 || h <= 0)
        return false; // minimized / mid-transition: skip the frame

    if ((uint32_t)w != user_data.surface_conf.width ||
        (uint32_t)h != user_data.surface_conf.height)
    {
        user_data.surface_conf.width = w;
        user_data.surface_conf.height = h;
        wgpuSurfaceConfigure(user_data.surface, &user_data.surface_conf);
    }
    return true;
}

int Sol_Render_Tick()
{
    wgpuInstanceProcessEvents(user_data.instance);
    if (!user_data.ready || !Sync_Size())
        return 1;

    WGPUSurfaceTexture st = WGPU_SURFACE_TEXTURE_INIT;
    wgpuSurfaceGetCurrentTexture(user_data.surface, &st);

    switch (st.status)
    {
    case WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal:
        break;
    case WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal:
        if (st.texture)
            wgpuTextureRelease(st.texture);
        wgpuSurfaceConfigure(user_data.surface, &user_data.surface_conf);
        return 1;
    default:
        if (st.texture)
            wgpuTextureRelease(st.texture);
        wgpuSurfaceConfigure(user_data.surface, &user_data.surface_conf);
        return 1;
    }
    if (!st.texture)
        return 1;

    WGPUTextureView view = wgpuTextureCreateView(st.texture, nullptr);

    WGPUCommandEncoderDescriptor enc_desc = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(user_data.device, &enc_desc);

    WGPURenderPassColorAttachment ca = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
    ca.view = view;
    ca.loadOp = WGPULoadOp_Clear;
    ca.storeOp = WGPUStoreOp_Store;
    ca.clearValue = {0.0, 0.0, 0.0, 1.0};

    WGPURenderPassDescriptor rp_desc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
    rp_desc.colorAttachmentCount = 1;
    rp_desc.colorAttachments = &ca;

    WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &rp_desc);

    wgpuRenderPassEncoderSetPipeline(pass, user_data.pipeline);
    wgpuRenderPassEncoderDraw(pass, 3, 1, 0, 0);

    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);

    WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
    wgpuCommandEncoderRelease(encoder);

    wgpuQueueSubmit(user_data.queue, 1, &cmd);
    wgpuCommandBufferRelease(cmd);

#ifndef __EMSCRIPTEN__
    wgpuSurfacePresent(user_data.surface);
#endif

    wgpuTextureViewRelease(view);
    wgpuTextureRelease(st.texture);

    return 0;
}

int Sol_Render_Deinit()
{
    if (user_data.surface)
    {
        wgpuSurfaceUnconfigure(user_data.surface);
        wgpuSurfaceRelease(user_data.surface);
    }
    if (user_data.queue)
        wgpuQueueRelease(user_data.queue);
    if (user_data.device)
        wgpuDeviceRelease(user_data.device);
    if (user_data.adapter)
        wgpuAdapterRelease(user_data.adapter);
    if (user_data.instance)
        wgpuInstanceRelease(user_data.instance);

    return 0;
}