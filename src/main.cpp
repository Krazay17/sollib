#include <iostream>
#include "webgpu/webgpu.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#ifdef _WIN32
#include "Windows.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include "GLFW/glfw3.h"
#ifdef _WIN32
#include "GLFW/glfw3native.h"
#endif

#include "render/render.hpp"

GLFWwindow *window;
SolRender *solrender;

static void on_framebuffer_size(GLFWwindow *wd, int w, int h)
{
    if (solrender)
        solrender->Resize(w, h);
}

void main_tick()
{
    glfwPollEvents();
    solrender->Update();
}

int main()
{
    if (!glfwInit())
        return 1;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(800, 600, "Test", NULL, NULL);
    glfwSetFramebufferSizeCallback(window, on_framebuffer_size);
    int fw, fh;
    glfwGetFramebufferSize(window, &fw, &fh);

#ifdef __EMSCRIPTEN__
    solrender = new SolRender(nullptr, nullptr, fw, fh);
    emscripten_set_main_loop(main_tick, 0, true);
#else
    solrender = new SolRender(GetModuleHandle(nullptr), glfwGetWin32Window(window), fw, fh);
    while (!glfwWindowShouldClose(window))
        main_tick();

    delete solrender;
    glfwDestroyWindow(window);
    glfwTerminate();
#endif

    return 0;
}