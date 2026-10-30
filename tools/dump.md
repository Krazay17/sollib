```CPP
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <string_view>

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

#include "webgpu/webgpu.h"

// --- Helper: Convert WGPUStringView to std::string_view ---
std::string_view toStringView(WGPUStringView view) {
    if (view.data == nullptr) return {};
    return view.length == WGPU_STRLEN ? std::string_view(view.data) : std::string_view(view.data, view.length);
}

// --- Context Struct for Async Callbacks ---
struct UserData {
    bool requestEnded = false;
    WGPUAdapter adapter = nullptr;
    WGPUDevice device = nullptr;
};

// --- Sync Adapters & Devices Callbacks ---
void onAdapterEnded(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void* userdata1, void* userdata2) {
    auto* data = static_cast<UserData*>(userdata1);
    if (status == WGPURequestAdapterStatus_Success) {
        data->adapter = adapter;
    } else {
        std::cerr << "Failed to find an appropriate adapter: " << toStringView(message) << std::endl;
    }
    data->requestEnded = true;
}

void onDeviceEnded(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void* userdata1, void* userdata2) {
    auto* data = static_cast<UserData*>(userdata1);
    if (status == WGPURequestDeviceStatus_Success) {
        data->device = device;
    } else {
        std::cerr << "Failed to create WebGPU device: " << toStringView(message) << std::endl;
    }
    data->requestEnded = true;
}

int main() {
    // 1. Initialize GLFW & Create Window with NO API (for WebGPU/Vulkan)
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "SolVulk - WebGPU", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }

    // 2. Create WebGPU Instance
    WGPUInstanceDescriptor instanceDesc = {};
    WGPUInstance instance = wgpuCreateInstance(&instanceDesc);
    if (!instance) {
        std::cerr << "Failed to initialize WebGPU instance!" << std::endl;
        return 1;
    }
    std::cout << "WebGPU Instance created: " << instance << std::endl;

    UserData user_data;

    // 3. Synchronous Adapter Request
    WGPURequestAdapterOptions options = {};
    WGPURequestAdapterCallbackInfo adapterCallback = {
        .callback = onAdapterEnded,
        .userdata1 = &user_data
    };
    wgpuInstanceRequestAdapter(instance, &options, adapterCallback);

    // Poll instance events until the adapter callback fires
    while (!user_data.requestEnded) {
        wgpuInstanceProcessEvents(instance);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    user_data.requestEnded = false;

    if (!user_data.adapter) {
        std::cerr << "No WebGPU adapter found!" << std::endl;
        return 1;
    }

    // 4. Synchronous Device Request
    WGPUDeviceDescriptor deviceDesc = WGPU_DEVICE_DESCRIPTOR_INIT;
    WGPURequestDeviceCallbackInfo deviceCallback = {
        .callback = onDeviceEnded,
        .userdata1 = &user_data
    };
    wgpuAdapterRequestDevice(user_data.adapter, &deviceDesc, deviceCallback);

    while (!user_data.requestEnded) {
        wgpuInstanceProcessEvents(instance);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    WGPUDevice device = user_data.device;
    if (!device) {
        std::cerr << "Failed to create WebGPU logical device!" << std::endl;
        return 1;
    }
    WGPUQueue queue = wgpuDeviceGetQueue(device);
    std::cout << "WebGPU Device & Queue acquired successfully!" << std::endl;

    // 5. Create WebGPU Surface from GLFW Window (Win32 HWND)
    HWND hwnd = glfwGetWin32Window(window);
    HINSTANCE hinstance = GetModuleHandle(nullptr);

    // Note: depending on your exact wgpu-native header version, 
    // the sType uses WGPUSType_SurfaceSourceWindowsHWND or WGPUSType_SurfaceDescriptorFromWindowsHWND
    WGPUSurfaceSourceWindowsHWND winHwndDesc = {
        .chain = {
            .next = nullptr,
            .sType = WGPUSType_SurfaceSourceWindowsHWND,
        },
        .hinstance = hinstance,
        .hwnd = hwnd,
    };
    WGPUSurfaceDescriptor surfaceDesc = {
        .nextInChain = (WGPUChainedStruct*)&winHwndDesc,
    };
    WGPUSurface surface = wgpuInstanceCreateSurface(instance, &surfaceDesc);

    // 6. Configure Surface Swapchain
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    WGPUSurfaceConfiguration surfaceConfig = {
        .device = device,
        .format = WGPUTextureFormat_BGRA8Unorm, // Standard desktop backbuffer format
        .usage = WGPUTextureUsage_RenderAttachment,
        .viewFormatCount = 0,
        .viewFormats = nullptr,
        .alphaMode = WGPUCompositeAlphaMode_Auto,
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .presentMode = WGPUPresentMode_Fifo, // VSync enabled
    };
    wgpuSurfaceConfigure(surface, &surfaceConfig);

    // 7. Main Render Loop (Clearing screen to black)
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Handle window resizing updates if needed
        glfwGetWindowSize(window, &width, &height);
        if (width > 0 && height > 0) {
            surfaceConfig.width = width;
            surfaceConfig.height = height;
            // Reconfigure config if size changes dynamically
        }

        // Get the next frame texture view from the surface swapchain
        WGPUSurfaceTexture surfaceTexture;
        wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);
        if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal) {
            continue;
        }

        WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, nullptr);

        // Create command encoder for rendering commands
        WGPUCommandEncoderDescriptor encoderDesc = {};
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

        // Define render pass color attachment (Clear to solid black)
        WGPURenderPassColorAttachment colorAttachment = {
            .view = targetView,
            .resolveTarget = nullptr,
            .loadOp = WGPULoadOp_Clear,
            .storeOp = WGPUStoreOp_Store,
            .clearValue = {0.0, 0.0, 0.0, 1.0}, // RGBA: Solid Black
            .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
        };

        WGPURenderPassDescriptor renderPassDesc = {
            .colorAttachmentCount = 1,
            .colorAttachments = &colorAttachment,
            .depthStencilAttachment = nullptr,
        };

        WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
        wgpuRenderPassEncoderEnd(renderPass);
        wgpuRenderPassEncoderRelease(renderPass);

        // Finish encoding and submit command buffer to queue
        WGPUCommandBufferDescriptor cmdBufferDesc = {};
        WGPUCommandBuffer cmdBuffer = wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);
        wgpuCommandEncoderRelease(encoder);

        wgpuQueueSubmit(queue, 1, &cmdBuffer);
        wgpuCommandBufferRelease(cmdBuffer);

        // Present the frame to the window surface
        wgpuSurfacePresent(surface);

        // Process any underlying OS/wgpu events
        wgpuInstanceProcessEvents(instance);
        wgpuTextureViewRelease(targetView);
    }

    // Cleanup (good hygiene for engine architecture)
    wgpuSurfaceUnconfigure(surface);
    wgpuSurfaceRelease(surface);
    wgpuDeviceRelease(device);
    wgpuAdapterRelease(adapter);
    wgpuInstanceRelease(instance);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}