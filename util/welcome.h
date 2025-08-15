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
	Welcome()
		: frameCount(0), lastTime(0.0), fps(0), isEmbedded(false), nedLogoTexture(0),
		  hoveredThemeIndex(-1), hoverStartTime(0.0), isPreviewingTheme(false),
		  clickedThemeIndex(-1), clickAnimationStartTime(0.0),
		  isPlayingClickAnimation(false)
	{
		// Initialize welcome images
		welcomeImages[0] = {0, "Amber", "icons/amber-welcome.png", false};
		welcomeImages[1] = {0, "Solarized", "icons/solarized-welcome.png", false};
		welcomeImages[2] = {0, "Sol Light", "icons/sol-light-welcome.png", false};
		welcomeImages[3] = {0, "NED", "icons/ned-welcome.png", false};
	} // Initialize FPS members

	// FPS calculation members
	int frameCount;
	double lastTime;
	int fps;
	bool isEmbedded;

	// Logo texture
	GLuint nedLogoTexture;

	// Welcome theme images
	struct WelcomeImage
	{
		GLuint texture;
		std::string name;
		std::string filename;
		bool loaded;
	};
	WelcomeImage welcomeImages[4];

	// Theme preview state
	int hoveredThemeIndex;
	double hoverStartTime;
	bool isPreviewingTheme;
	std::string originalProfile;

	// Click animation state
	int clickedThemeIndex;
	double clickAnimationStartTime;
	bool isPlayingClickAnimation;

	void calculateFPS();	  // Helper to calculate FPS
	bool loadNedLogo();		  // Load the ned.png logo
	bool loadWelcomeImages(); // Load the welcome theme images
	void renderWelcomeImageGrid(float windowWidth,
								float windowHeight,
								float currentY); // Render the image grid
	void handleThemePreview(int themeIndex,
							bool isHovered,
							bool isClicked); // Handle theme preview logic
};

extern Welcome &gWelcome;