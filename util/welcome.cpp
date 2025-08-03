#include "welcome.h"
#include "../files/files.h"
#include "util/debug_console.h"
#include <iostream>

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

	ImGui::SetCursorPosY(windowHeight * 0.2f);
	{ // Scope for title scaling
		ImFont *currentFont = ImGui::GetIO().Fonts->Fonts[0];
		float baseFontSize = currentFont->FontSize;
		float desiredFontSize = 48.0f; // Title size
		float scaleFactor = (baseFontSize > 0) ? (desiredFontSize / baseFontSize) : 1.0f;

		ImGui::PushFont(currentFont);
		ImGui::SetWindowFontScale(scaleFactor);

		const char *title = "Welcome to NED";
		float titleWidth = ImGui::CalcTextSize(title).x;
		ImGui::SetCursorPosX((windowWidth - titleWidth) * 0.5f);
		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "%s", title);

		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopFont();
	} // End scope for title scaling

	// GitHub link right under title
	ImGui::SetCursorPosY(windowHeight * 0.27f); // Set Y position first
	{											// Scope for github link scaling
		ImFont *currentFont = ImGui::GetIO().Fonts->Fonts[0]; // Get default font again
		float baseFontSize = currentFont->FontSize;
		float desiredFontSize = 22.0f; // <<< Github link desired size
		float scaleFactor = (baseFontSize > 0) ? (desiredFontSize / baseFontSize) : 1.0f;

		ImGui::PushFont(currentFont);			// Push font to scale
		ImGui::SetWindowFontScale(scaleFactor); // Apply scaling for github link

		const char *github = "github.com/nealmick/ned";
		float githubWidth =
			ImGui::CalcTextSize(github).x; // Calculate width with scaled font
		ImGui::SetCursorPosX((windowWidth - githubWidth) *
							 0.5f); // Set X position after calculating width
		ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f),
						   "%s",
						   github); // Draw scaled text
		ImGui::SetItemAllowOverlap();

		ImGui::SetWindowFontScale(1.0f); // Reset scale
		ImGui::PopFont();				 // Pop font
	} // End scope for github link scaling
	if (ImGui::GetWindowWidth() > 750.0f)
	{
		ImGui::SetCursorPosY(windowHeight * 0.35f);
		{ // Scope for description scaling
			ImFont *currentFont = ImGui::GetIO().Fonts->Fonts[0];
			float baseFontSize = currentFont->FontSize;
			float desiredFontSize = 24.0f; // <<< Description desired size
			float scaleFactor =
				(baseFontSize > 0) ? (desiredFontSize / baseFontSize) : 1.0f;

			ImGui::PushFont(currentFont);
			ImGui::SetWindowFontScale(scaleFactor);

			const char *description = "A lightweight, feature-rich text editor "
									  "built with C++ and ImGui.";
			float descWidth = ImGui::CalcTextSize(description).x;
			ImGui::SetCursorPosX((windowWidth - descWidth) * 0.5f);
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", description);

			ImGui::SetWindowFontScale(1.0f);
			ImGui::PopFont();
		} // End scope for description scaling

		// Keybinds section
		ImGui::SetCursorPosY(windowHeight * 0.42f);
		{ // Scope for keybinds scaling
			ImFont *currentFont = ImGui::GetIO().Fonts->Fonts[0];
			float baseFontSize = currentFont->FontSize;
			float desiredFontSize = 24.0f; // <<< Keybinds desired size
			float scaleFactor =
				(baseFontSize > 0) ? (desiredFontSize / baseFontSize) : 1.0f;

			ImGui::PushFont(currentFont);
			ImGui::SetWindowFontScale(scaleFactor); // Apply scale for the whole section

			// First row of keybinds
			ImGui::SetCursorPosX(windowWidth * 0.3f);
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "CMD+O Open Folder");
			ImGui::SameLine(windowWidth * 0.6f);
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "CMD+T Terminal");

			// Second row
			ImGui::SetCursorPosX(windowWidth * 0.3f);
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "CMD+B Bookmarks");
			ImGui::SameLine(windowWidth * 0.6f);
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "CMD+: Line Jump");

			// Third row
			ImGui::SetCursorPosX(windowWidth * 0.3f);
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "CMD+F Find");
			ImGui::SameLine(windowWidth * 0.6f);
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "CMD+/ Show this window");

			ImGui::SetWindowFontScale(1.0f); // Reset scale after the section
			ImGui::PopFont();				 // Pop font after the section
		} // End scope for keybinds scaling
	}
	// Open Folder button
	float buttonWidth = 300;
	float buttonHeight = 50;
	if (ImGui::GetWindowWidth() < 750.0f)
	{
		ImGui::SetCursorPosY(windowHeight * 0.40f);
	} else
	{
		ImGui::SetCursorPosY(windowHeight * 0.60f);
	}
	ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.8f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.9f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.7f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

	{ // Scope for button text scaling
		ImFont *currentFont = ImGui::GetIO().Fonts->Fonts[0];
		float baseFontSize = currentFont->FontSize;
		float desiredFontSize = 32.0f; // <<< Button text desired size
		float scaleFactor = (baseFontSize > 0) ? (desiredFontSize / baseFontSize) : 1.0f;

		ImGui::PushFont(currentFont);
		ImGui::SetWindowFontScale(scaleFactor); // Scale font for the button text

		// Draw the button - the text inside will use the scaled font
		if (ImGui::Button("Open Folder", ImVec2(buttonWidth, buttonHeight)))
		{
			std::cout << "\033[32mMain:\033[0m Welcome screen - Open Folder clicked"
					  << std::endl;
			gFileExplorer._showFileDialog = true;
		}

		ImGui::SetWindowFontScale(1.0f); // Reset scale immediately after button
		ImGui::PopFont();				 // Pop font immediately after button
	} // End scope for button text scaling

	ImGui::SetCursorPosY(windowHeight * 0.71f); // Position below button
	ImGui::SetCursorPosX((windowWidth - (windowWidth * 0.6f)) * 0.5f); // Center console

	ImGui::SetCursorPosY(windowHeight * 0.71f); // Position below button
	ImGui::SetCursorPosX((windowWidth - (windowWidth * 0.6f)) * 0.5f); // Center console
	gDebugConsole.render();

	ImGui::PopStyleVar();
	ImGui::PopStyleColor(3);

	if (!isEmbedded)
	{
		ImGui::End();
	}
}