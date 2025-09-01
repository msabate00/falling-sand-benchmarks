#pragma once
#include <vector>
#include <glad/glad.h>   
#include <GLFW/glfw3.h> 
#include "particle.h"

class Engine {
public:
    Engine(int width, int height);
    void update();
    void draw();
private:
    int width, height;
    std::vector<Particle> particles;
};
