#include "welcome.h"
#include "../files/files.h"
#include "util/debug_console.h"
#include <iostream>

// PNG loading with stb_image
#define STB_IMAGE_IMPLEMENTATION
#include "../lib/stb_image.h"

Welcome &gWelcome = Welcome::getInstance();

void Welcome::calculateFPS()
{
	frameCount++;
	double currentTime = glfwGetTime();

	if (currentTime - lastTime >= 1.0)
	{
		fps = frameCount;
		frameCount = 0;
		lastTime = currentTime;
	}
}

bool Welcome::loadNedLogo()
{
	if (nedLogoTexture != 0)
		return true; // Already loaded

	int width, height, channels;
	unsigned char *data = stbi_load("icons/ned.png", &width, &height, &channels, 4);
	if (!data)
	{
		std::cerr << "Failed to load ned.png logo" << std::endl;
		return false;
	}

	// Create OpenGL texture
	glGenTextures(1, &nedLogoTexture);
	glBindTexture(GL_TEXTURE_2D, nedLogoTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	stbi_image_free(data);
	return true;
}

void Welcome::render()
{
	calculateFPS(); // Update FPS count

	float windowWidth, windowHeight;

	if (isEmbedded)
	{
		// When embedded, use the current content area
		windowWidth = ImGui::GetContentRegionAvail().x;
		windowHeight = ImGui::GetContentRegionAvail().y;
	} else
	{
		// When standalone, create a full window
		ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);

		ImGui::Begin("##WelcomeScreen",
					 nullptr,
					 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
						 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

		windowWidth = ImGui::GetWindowWidth();
		windowHeight = ImGui::GetWindowHeight();
	}

	// FPS Counter in top right
	ImGui::SetCursorPos(ImVec2(windowWidth - 80, 10));
	ImVec4 fpsColor;
	if (fps >= 55)
	{
		fpsColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green for good FPS
	} else if (fps >= 30)
	{
		fpsColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow for okay FPS
	} else
	{
		fpsColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red for bad FPS
	}
	// ImGui::TextColored(fpsColor, "FPS: %d", fps);

	// Dynamic vertical positioning - move up for smaller windows
	float minTopMargin = 20.0f;
	float maxTopMargin = 80.0f;
	float topMargin = minTopMargin + (maxTopMargin - minTopMargin) *
										 std::min(1.0f, (windowHeight - 400.0f) / 400.0f);
	float currentY = topMargin;

	// Check if we have enough width for side-by-side layout
	bool useSideBySideLayout = windowWidth > 800.0f;

	if (useSideBySideLayout && loadNedLogo())
	{
		// Hero layout: Logo on LEFT of center, title and button on RIGHT of center
		// Scale logo based on window size - bigger windows get bigger logos (40% larger)
		float logoSize =
			std::min(windowWidth * 0.31f, std::min(310.0f, windowHeight * 0.49f));
		float centerX = windowWidth * 0.5f;
		float spacing = 60.0f; // Space between logo and title/button section

		// Logo positioned LEFT of center
		float logoX = centerX - spacing - logoSize;
		float logoY = currentY + 20.0f;

		ImGui::SetCursorPos(ImVec2(logoX, logoY));
		ImGui::Image((ImTextureID)(intptr_t)nedLogoTexture, ImVec2(logoSize, logoSize));

		// Title and button positioned RIGHT of center, moved 200px left
		float contentStartX = centerX + spacing - 100.0f;
		float titleY = logoY + 20.0f;

		// Title - positioned right of center
		{ // Scope for title scaling
			ImFont *currentFont = ImGui::GetIO().Fonts->Fonts[0];
			float baseFontSize = currentFont->LegacySize;
			float desiredFontSize = std::min(64.0f, windowWidth * 0.12f);
			float scaleFactor =
				(baseFontSize > 0) ? (desiredFontSize / baseFontSize) : 1.0f;

			ImGui::PushFont(currentFont);
			ImGui::SetWindowFontScale(scaleFactor);

			const char *title = "Welcome to NED";
			ImGui::SetCursorPos(ImVec2(contentStartX, titleY));
			ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "%s", title);

			ImGui::SetWindowFontScale(1.0f);
			ImGui::PopFont();
		}

		// Button below title - positioned right of center (bigger button)
		float buttonY = titleY + 70.0f;
		float buttonWidth = 280.0f;
		float buttonHeight = 60.0f;
		float buttonX = contentStartX;

		ImGui::SetCursorPos(ImVec2(buttonX, buttonY));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.8f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.9f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.7f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

		{ // Scope for button text scaling
			ImFont *currentFont = ImGui::GetIO().Fonts->Fonts[0];
			float baseFontSize = currentFont->LegacySize;
			float desiredFontSize = 26.0f;
			float scaleFactor =
				(baseFontSize > 0) ? (desiredFontSize / baseFontSize) : 1.0f;

			ImGui::PushFont(currentFont);
			ImGui::SetWindowFontScale(scaleFactor);

			if (ImGui::Button("Open Folder", ImVec2(buttonWidth, buttonHeight)))
			{
				std::cout << "\033[32mMain:\033[0m Welcome screen - Open Folder clicked"
						  << std::endl;
				gFileExplorer._showFileDialog = true;
			}

			ImGui::SetWindowFontScale(1.0f);
			ImGui::PopFont();
		}

		ImGui::PopStyleVar();
		ImGui::PopStyleColor(3);

		// Keybinds section under the button on the right side
		float keybindsY = buttonY + buttonHeight + 30.0f;
		float keybindsX = contentStartX;

		// Only show keybinds if there's enough space
		if (windowHeight > keybindsY + 120.0f)
		{
			{ // Scope for keybinds scaling
				ImFont *currentFont = ImGui::GetIO().Fonts->Fonts[0];
				float baseFontSize = currentFont->LegacySize;
				float desiredFontSize = std::min(20.0f, windowWidth * 0.045f);
				float scaleFactor =
					(baseFontSize > 0) ? (desiredFontSize / baseFontSize) : 1.0f;

				ImGui::PushFont(currentFont);
				ImGui::SetWindowFontScale(scaleFactor);

				// Two column layout aligned under the button
				const char *keybinds[] = {"CMD+O Open Folder",
										  "CMD+T Terminal",
										  "CMD+B Bookmarks",
										  "CMD+: Line Jump",
										  "CMD+F Find",
										  "CMD+/ Show this window"};

				// Two-column layout: 3 rows, 2 columns
				float colSpacing = 200.0f;
				for (int i = 0; i < 3; i++)
				{
					// Left column
					ImGui::SetCursorPos(ImVec2(keybindsX, keybindsY + i * 22.0f));
					ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
									   "%s",
									   keybinds[i * 2]);

					// Right column
					if (i * 2 + 1 < 6)
					{
						ImGui::SetCursorPos(
							ImVec2(keybindsX + colSpacing, keybindsY + i * 22.0f));
						ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
										   "%s",
										   keybinds[i * 2 + 1]);
					}
				}

				ImGui::SetWindowFontScale(1.0f);
				ImGui::PopFont();
			}
		}

		currentY = std::max(logoY + logoSize,
							std::max(buttonY + buttonHeight, keybindsY + 140.0f)) +
				   40.0f;
	} else
	{
		// Vertical layout: Logo, title, and button stacked

		// Logo (if available and fits)
		if (loadNedLogo() && windowHeight > 400.0f)
		{
			float logoSize =
				std::min(windowWidth * 0.3f, std::min(150.0f, windowHeight * 0.2f));
			float logoX = (windowWidth - logoSize) * 0.5f;

			ImGui::SetCursorPos(ImVec2(logoX, currentY));
			ImGui::Image((ImTextureID)(intptr_t)nedLogoTexture,
						 ImVec2(logoSize, logoSize));
			currentY += logoSize + 30.0f;
		}

		// Title - always centered
		{ // Scope for title scaling
			ImFont *currentFont = ImGui::GetIO().Fonts->Fonts[0];
			float baseFontSize = currentFont->LegacySize;
			float desiredFontSize = std::min(40.0f, windowWidth * 0.08f);
			float scaleFactor =
				(baseFontSize > 0) ? (desiredFontSize / baseFontSize) : 1.0f;

			ImGui::PushFont(currentFont);
			ImGui::SetWindowFontScale(scaleFactor);

			const char *title = "Welcome to NED";
			float titleWidth = ImGui::CalcTextSize(title).x;
			ImGui::SetCursorPos(ImVec2((windowWidth - titleWidth) * 0.5f, currentY));
			ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "%s", title);

			ImGui::SetWindowFontScale(1.0f);
			ImGui::PopFont();
		}
		currentY += 60.0f;

		// Button - centered (bigger in vertical layout too)
		float buttonWidth = std::min(320.0f, windowWidth * 0.8f);
		float buttonHeight = 55.0f;

		ImGui::SetCursorPos(ImVec2((windowWidth - buttonWidth) * 0.5f, currentY));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.8f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.9f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.7f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

		{ // Scope for button text scaling
			ImFont *currentFont = ImGui::GetIO().Fonts->Fonts[0];
			float baseFontSize = currentFont->LegacySize;
			float desiredFontSize = 24.0f;
			float scaleFactor =
				(baseFontSize > 0) ? (desiredFontSize / baseFontSize) : 1.0f;

			ImGui::PushFont(currentFont);
			ImGui::SetWindowFontScale(scaleFactor);

			if (ImGui::Button("Open Folder", ImVec2(buttonWidth, buttonHeight)))
			{
				std::cout << "\033[32mMain:\033[0m Welcome screen - Open Folder clicked"
						  << std::endl;
				gFileExplorer._showFileDialog = true;
			}

			ImGui::SetWindowFontScale(1.0f);
			ImGui::PopFont();
		}

		ImGui::PopStyleVar();
		ImGui::PopStyleColor(3);

		currentY += buttonHeight + 40.0f;
	}

	// GitHub link at the bottom - centered
	float githubY = windowHeight - 60.0f; // 60px from bottom
	ImGui::SetCursorPosY(githubY);
	{ // Scope for github link scaling
		ImFont *currentFont = ImGui::GetIO().Fonts->Fonts[0];
		float baseFontSize = currentFont->LegacySize;
		float desiredFontSize = 20.0f;
		float scaleFactor = (baseFontSize > 0) ? (desiredFontSize / baseFontSize) : 1.0f;

		ImGui::PushFont(currentFont);
		ImGui::SetWindowFontScale(scaleFactor);

		const char *github = "github.com/nealmick/ned";
		float githubWidth = ImGui::CalcTextSize(github).x;
		ImGui::SetCursorPosX((windowWidth - githubWidth) * 0.5f);

		// Make GitHub link clickable
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
		if (ImGui::Selectable(github,
							  false,
							  ImGuiSelectableFlags_None,
							  ImVec2(githubWidth,
									 ImGui::GetTextLineHeight() * scaleFactor)))
		{
			// Open GitHub URL in default browser
#ifdef __APPLE__
			system("open https://github.com/nealmick/ned");
#elif defined(_WIN32)
			system("start https://github.com/nealmick/ned");
#else
			system("xdg-open https://github.com/nealmick/ned");
#endif
		}
		ImGui::PopStyleColor();

		// Add hover effect
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Click to open GitHub repository");
		}

		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopFont();
	} // End scope for github link scaling

	if (!isEmbedded)
	{
		ImGui::End();
	}
}