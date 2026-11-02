#include "core/core.hpp"
#include <chrono>
#include <thread>

#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

GLFWwindow *window;
SolCore sol_core;
double prev_time = 0;

static void on_framebuffer_size(GLFWwindow *wd, int w, int h)
{
    sol_core.Resize(w, h);
}

void main_tick()
{
    glfwPollEvents();
    double time = glfwGetTime();
    double dt = time - prev_time;
    prev_time = time;
    sol_core.Tick(dt);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void start_tick()
{
    while (sol_core.running && !glfwWindowShouldClose(window))
        main_tick();
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

    HWND hwnd = glfwGetWin32Window(window);
    HINSTANCE hinstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
    sol_core.gfx.Init(hinstance, hwnd, fw, fh);
    sol_core.net.Init();
    sol_core.Start();

    start_tick();

    sol_core.Exit();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
