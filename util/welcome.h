#pragma once
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <string>

class FileExplorer;
class Settings;

class Welcome
{
  public:
	Welcome(Settings &settings, FileExplorer &fileExplorer);
	void render();

  private:
	GLuint nedLogoTexture = 0;

	struct WelcomeImage
	{
		GLuint texture = 0;
		std::string name;
		std::string filename;
		bool loaded = false;
	};
	WelcomeImage welcomeImages[4] = {
		{0, "Amber", "resources/icons/amber-welcome.png", false},
		{0, "Solarized", "resources/icons/solarized-welcome.png", false},
		{0, "Sol Light", "resources/icons/sol-light-welcome.png", false},
		{0, "NED", "resources/icons/ned-welcome.png", false},
	};

	int clickedThemeIndex = -1;
	double clickAnimationStartTime = 0.0;
	bool isPlayingClickAnimation = false;

	Settings &settings;
	FileExplorer &fileExplorer;

	bool loadNedLogo();
	bool loadWelcomeImages();
	void renderWelcomeImageGrid(float windowWidth, float windowHeight, float currentY);
	void selectTheme(int themeIndex);
};
