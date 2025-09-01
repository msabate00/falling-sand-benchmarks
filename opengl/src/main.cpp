#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include "engine.h"
#include "material.h"

static int winW = 1280, winH = 720;
static int gridW = 320, gridH = 180; // ajusta relación aspecto

static double lastX = 0.0, lastY = 0.0;
static bool lmbDown = false;

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        lmbDown = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(winW, winH, "FallingSand", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    if (gladLoaderLoadGL() == 0) return -1;
    glfwSwapInterval(1);

    glfwSetMouseButtonCallback(window, mouse_button_callback);

    Engine engine(gridW, gridH);

    auto t0 = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        auto t1 = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(t1 - t0).count();
        t0 = t1;

        // input -> pintar arena
        double mx, my; glfwGetCursorPos(window, &mx, &my);
        lastX = mx; lastY = my;

        // convertir a coords de grid
        int gx = int((mx / double(winW)) * gridW);
        int gy = int((my / double(winH)) * gridH);
        if (lmbDown) {

            engine.paint(gx, gy, Material::Sand, 4); // radio 4
        }

        engine.update(dt);

        glViewport(0, 0, winW, winH);
        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        engine.draw();

        glfwSwapBuffers(window);
        glfwGetWindowSize(window, &winW, &winH);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
