#include "engine.h"
#include <iostream>
#include <gl/GL.h>

Engine::Engine(int w, int h) : width(w), height(h) {


    for (int i = 0;i < 100;i++) {
        Particle p;
        p.x = static_cast<float>(rand() % width) / width * 2.0f - 1.0f;
        p.y = static_cast<float>(rand() % height) / height * 2.0f - 1.0f;
        p.vx = 0.0f;
        p.vy = -0.01f; // gravedad hacia abajo
        p.r = 1.0f;
        p.g = 1.0f;
        p.b = 0.0f; // amarillo
        particles.push_back(p);
    }


}

void Engine::update() {
    // Aquí iría la lógica de partículas
    for (auto& p : particles) {
        p.y += p.vy;
        p.x += p.vx;
    }
}

void Engine::draw() {
    // Aquí iría OpenGL para dibujar partículas
    glBegin(GL_POINTS);
    for (auto& p : particles) {
        glColor3f(p.r, p.g, p.b);
        glVertex2f(p.x, p.y);
    }
    glEnd();


}
