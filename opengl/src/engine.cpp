#include "engine.h"
#include <iostream>

Engine::Engine(int w, int h) : width(w), height(h) {}

void Engine::update() {
    // Aquí iría la lógica de partículas
    for (auto& p : particles) {
        p.y += p.vy;
        p.x += p.vx;
    }
}

void Engine::draw() {
    // Aquí iría OpenGL para dibujar partículas
}
