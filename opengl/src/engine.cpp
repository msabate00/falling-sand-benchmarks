#include "engine.h"
#include <iostream>

Engine::Engine(int w, int h) : width(w), height(h) {}

void Engine::update() {
    // Aqu� ir�a la l�gica de part�culas
    for (auto& p : particles) {
        p.y += p.vy;
        p.x += p.vx;
    }
}

void Engine::draw() {
    // Aqu� ir�a OpenGL para dibujar part�culas
}
