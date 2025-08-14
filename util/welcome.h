// welcome.h
#pragma once
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <string>

class Welcome
{
  public:
	void render(); // Main render function for welcome screen
	static Welcome &getInstance()
	{
		static Welcome instance;
		return instance;
	}

	void setEmbedded(bool embedded) { isEmbedded = embedded; }
	bool getEmbedded() const { return isEmbedded; }

  private:
	Welcome() : frameCount(0), lastTime(0.0), fps(0), isEmbedded(false), nedLogoTexture(0)
	{
	} // Initialize FPS members

	// FPS calculation members
	int frameCount;
	double lastTime;
	int fps;
	bool isEmbedded;

	// Logo texture
	GLuint nedLogoTexture;

	void calculateFPS(); // Helper to calculate FPS
	bool loadNedLogo();	 // Load the ned.png logo
};

extern Welcome &gWelcome;