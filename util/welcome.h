// welcome.h
#pragma once
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <string>

class Welcome {
  public:
    void render(); // Main render function for welcome screen
    static Welcome &getInstance() {
        static Welcome instance;
        return instance;
    }

  private:
    Welcome() : frameCount(0), lastTime(0.0), fps(0) {} // Initialize FPS members

    // FPS calculation members
    int frameCount;
    double lastTime;
    int fps;

    void calculateFPS(); // Helper to calculate FPS
};

extern Welcome &gWelcome;