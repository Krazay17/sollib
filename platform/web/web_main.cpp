#include "sol/sol.hpp"
#include <chrono>
#include <thread>

#include "GLFW/glfw3.h"
#include <emscripten.h>
#include <emscripten/html5.h>

GLFWwindow *window;
SolCore sol_core;
double prev_time = 0;

static void on_framebuffer_size(GLFWwindow *wd, int w, int h)
{
    sol_core.Resize(w, h);
}

void main_tick()
{
    double time = glfwGetTime();
    double dt = time - prev_time;
    prev_time = time;
    sol_core.Tick(dt);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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

    sol_core.Render_Init(nullptr, window, fw, fh);
    emscripten_set_main_loop(main_tick, 0, true);

    return 0;
}