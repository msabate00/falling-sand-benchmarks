#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include "engine.h"
#include "material.h"
#include "renderer.h"
#include "ui.h"
#include "audio.h"

static int winW = 1280, winH = 720;
static int gridW = 320, gridH = 180;

static bool lmbDown = false;
static Material brushMat = Material::Sand;
static int brushSize = 4;

static Engine engine = Engine(gridW, gridH);
static Renderer* renderer = nullptr;
static UI ui;
static Audio audio;

static void mouse_button_callback(GLFWwindow* w, int b, int a, int m) {
    if (b == GLFW_MOUSE_BUTTON_LEFT) lmbDown = (a != GLFW_RELEASE);
}
static void scroll_callback(GLFWwindow*, double, double yoff) {
    brushSize += (int)yoff; if (brushSize < 1) brushSize = 1;
}
static void key_callback(GLFWwindow*, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    switch (key) {
    case GLFW_KEY_1: brushMat = Material::Sand;  break;
    case GLFW_KEY_2: brushMat = Material::Water; break;
    case GLFW_KEY_3: brushMat = Material::Stone; break;
    case GLFW_KEY_4: brushMat = Material::Wood;  break;
    case GLFW_KEY_5: brushMat = Material::Fire;  break;
    case GLFW_KEY_6: brushMat = Material::Smoke; break;
    case GLFW_KEY_9: brushMat = Material::Empty; break;
    case GLFW_KEY_P: engine.paused = !engine.paused; break;
    case GLFW_KEY_N: engine.stepOnce = true; break;
    default: break;
    }
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(winW, winH, "FallingSand", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    if (!gladLoadGL(glfwGetProcAddress)) return -1;
    glfwSwapInterval(1);

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    engine = Engine(gridW, gridH);
    renderer = new Renderer();

    audio.init();
    audio.load("ignite", AUDIO_DIR  "/ignite.wav", 16);
    audio.load("paint", AUDIO_DIR  "/paint.wav", 8);

    ui.init();

    auto t0 = std::chrono::high_resolution_clock::now();
    double fpsTimer = 0.0;
    int frames = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        auto t1 = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(t1 - t0).count();
        t0 = t1;

        double mx, my; glfwGetCursorPos(window, &mx, &my);
        int gx = int((mx / double(winW)) * gridW);
        int gy = int((my / double(winH)) * gridH);
        ui.setMouse(mx, my, lmbDown);

        engine.update(dt);
        std::vector<AudioEvent> evs;
        if (engine.takeAudioEvents(evs)) {
            for (const auto& e : evs) {
                float x01 = float(e.x) / float(gridW);
                float y01 = float(e.y) / float(gridH);
                switch (e.type) {
                case AudioEvent::Type::Ignite: audio.play("ignite", x01, y01, 0.9f); break;
                case AudioEvent::Type::Paint:  audio.play("paint", x01, y01, 0.5f); break;
                }
            }
        }

        int rx = 0, ry = 0, rw = 0, rh = 0;
        bool hasDirty = engine.takeDirtyRect(rx, ry, rw, rh);
        if (!hasDirty) { rw = rh = 0; }

        renderer->drawPlane(engine.planeM(), gridW, gridH, winW, winH, rx, ry, rw, rh);

        ui.begin(winW, winH);
        
        ui.draw(engine, brushSize, brushMat);

        ui.end();

        if (lmbDown && !ui.consumedMouse()) engine.paint(gx, gy, brushMat, brushSize);

        frames++;
        fpsTimer += dt;
        if (fpsTimer >= 1.0) {
            double fps = frames / fpsTimer;
            char buf[128];
            std::snprintf(buf, sizeof(buf), "FallingSand - %.1f FPS", fps);
            glfwSetWindowTitle(window, buf);
            fpsTimer = 0.0;
            frames = 0;
        }

        glfwSwapBuffers(window);
        glfwGetWindowSize(window, &winW, &winH);
    }
    ui.shutdown();
    audio.shutdown();

    return 0;
}
