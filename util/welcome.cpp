#include "welcome.h"
#include "../files/files.h"
#include "macos_window.h"
#include "settings.h"
#include <filesystem>
#include <iostream>

// PNG loading with stb_image
#define STB_IMAGE_IMPLEMENTATION
#include "../lib/stb_image.h"

namespace {
ImVec4 colorFromJson(const json &color)
{
	return ImVec4(color[0], color[1], color[2], color[3]);
}
} // namespace

Welcome::Welcome(Settings &settings, FileExplorer &fileExplorer)
	: settings(settings), fileExplorer(fileExplorer)
{
}

bool Welcome::loadNedLogo()
{
	if (nedLogoTexture != 0)
		return true; // Already loaded

	const std::string logoPath = (std::filesystem::path(Settings::getAppResourcesPath()) /
								  "resources" / "icons" / "ned.png")
									 .string();
	int width, height, channels;
	unsigned char *data = stbi_load(logoPath.c_str(), &width, &height, &channels, 4);
	if (!data)
	{
		// Dev fallback when cwd is the repo root.
		data = stbi_load("resources/icons/ned.png", &width, &height, &channels, 4);
	}
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

bool Welcome::loadWelcomeImages()
{
	bool allLoaded = true;

	for (int i = 0; i < 4; i++)
	{
		if (welcomeImages[i].loaded)
			continue; // Already loaded

		int width, height, channels;
		const std::string absPath =
			(std::filesystem::path(Settings::getAppResourcesPath()) /
			 welcomeImages[i].filename)
				.string();
		unsigned char *data = stbi_load(absPath.c_str(), &width, &height, &channels, 4);
		if (!data)
			data = stbi_load(
				welcomeImages[i].filename.c_str(), &width, &height, &channels, 4);
		if (!data)
		{
			std::cerr << "Failed to load " << welcomeImages[i].filename << std::endl;
			allLoaded = false;
			continue;
		}

		// Create OpenGL texture
		glGenTextures(1, &welcomeImages[i].texture);
		glBindTexture(GL_TEXTURE_2D, welcomeImages[i].texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		stbi_image_free(data);
		welcomeImages[i].loaded = true;
	}

	return allLoaded;
}

void Welcome::renderWelcomeImageGrid(float windowWidth, float windowHeight, float currentY)
{
	// Only show image grid if window is wide enough and we have space
	if (windowWidth < 600.0f || !loadWelcomeImages())
		return;

	// Check if we have enough vertical space
	float remainingHeight =
		windowHeight - currentY - 100.0f; // Leave space for github link
	if (remainingHeight < 150.0f)
		return;

	// Calculate responsive image size based on window width (35% smaller)
	// Original images are 1240x940 (aspect ratio ~1.32:1)
	float effectiveWidth = std::min(windowWidth, 1100.0f); // Cap scaling at 1100px
	float baseImageSize =
		std::min(160.0f, effectiveWidth * 0.1f);	  // Reduced from 250px to 160px
	float imageSize = std::max(80.0f, baseImageSize); // Reduced minimum size

	// Scale up for larger windows (less aggressive scaling) - but only up to 1100px width
	if (effectiveWidth > 1000.0f)
		imageSize = baseImageSize * 1.1f;

	// Calculate grid layout
	float spacing = 20.0f;
	float totalGridWidth = (imageSize * 4) + (spacing * 3);

	// If grid doesn't fit, make images smaller
	if (totalGridWidth > windowWidth - 40.0f)
	{
		imageSize = (windowWidth - 40.0f - (spacing * 3)) / 4.0f;
		if (imageSize < 80.0f) // Too small, don't show
			return;
	}

	// Center the grid
	float startX = (windowWidth - totalGridWidth) * 0.5f;
	if (totalGridWidth > windowWidth - 40.0f)
	{
		totalGridWidth = (imageSize * 4) + (spacing * 3);
		startX = (windowWidth - totalGridWidth) * 0.5f;
	}

	// Add some padding above the grid
	currentY += 30.0f;

	// Render the 4 images in a row
	for (int i = 0; i < 4; i++)
	{
		if (!welcomeImages[i].loaded)
			continue;

		float imageX = startX + (imageSize + spacing) * i;
		ImGui::SetCursorPos(ImVec2(imageX, currentY));

		// Create an invisible button for click detection
		ImGui::PushStyleColor(ImGuiCol_Button,
							  ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
							  ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent hover
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,
							  ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent active

		std::string buttonId = "welcome_img_" + std::to_string(i);
		bool clicked = ImGui::Button(buttonId.c_str(), ImVec2(imageSize, imageSize));

		ImGui::PopStyleColor(3);

		// Get the button's position for drawing
		ImVec2 rectMin = ImGui::GetItemRectMin();
		ImVec2 rectMax = ImGui::GetItemRectMax();
		ImDrawList *draw_list = ImGui::GetWindowDrawList();

		// Draw the rounded image
		draw_list->AddImageRounded((ImTextureID)(intptr_t)welcomeImages[i].texture,
								   rectMin,
								   rectMax,
								   ImVec2(0, 0),
								   ImVec2(1, 1),   // UV coordinates (full image)
								   IM_COL32_WHITE, // Tint color
								   12.0f);		   // Rounding radius

		// Draw hover overlay if hovered
		bool isHovered = ImGui::IsItemHovered();
		if (isHovered)
		{
			draw_list->AddRectFilled(
				rectMin, rectMax, IM_COL32(0, 123, 255, 76), 12.0f); // Light blue overlay
		}

		// Always draw grey border
		draw_list->AddRect(rectMin, rectMax, IM_COL32(128, 128, 128, 80), 12.0f, 0, 1.5f);

		// Check for click animation on this image
		bool showClickAnimation = false;
		float clickAnimationAlpha = 0.0f;
		if (isPlayingClickAnimation && clickedThemeIndex == i)
		{
			double currentTime = glfwGetTime();
			double animationProgress =
				(currentTime - clickAnimationStartTime) / 0.7; // 0.7 second animation

			if (animationProgress <= 1.0)
			{
				// Create fade in/out effect - reaches peak at 0.5, fades to 0 at 1.0
				if (animationProgress <= 0.5)
				{
					clickAnimationAlpha = animationProgress * 2.0f; // Fade in (0 to 1)
				} else
				{
					clickAnimationAlpha =
						2.0f - (animationProgress * 2.0f); // Fade out (1 to 0)
				}
				showClickAnimation = true;
			} else
			{
				// Animation finished
				isPlayingClickAnimation = false;
				clickedThemeIndex = -1;
			}
		}

		// Draw click animation border (white)
		if (showClickAnimation)
		{
			uint8_t alpha = (uint8_t)(clickAnimationAlpha * 255.0f);
			draw_list->AddRect(
				rectMin, rectMax, IM_COL32(255, 255, 255, alpha), 12.0f, 0, 3.0f);
		}

		// Add blue border on hover (only if not showing click animation)
		if (isHovered && !showClickAnimation)
		{
			draw_list->AddRect(
				rectMin, rectMax, IM_COL32(0, 123, 255, 200), 12.0f, 0, 2.5f);

			// Create tooltip with theme background color (force opacity to 1.0)
			ImVec4 bgColor = colorFromJson(settings.settings["backgroundColor"]);
			bgColor.w = 1.0f; // Force full opacity
			ImGui::PushStyleColor(ImGuiCol_PopupBg, bgColor);
			ImGui::BeginTooltip();
			ImGui::Text("%s", welcomeImages[i].name.c_str());
			ImGui::EndTooltip();
			ImGui::PopStyleColor();
		}

		if (clicked)
			selectTheme(i);
	}
}

void Welcome::selectTheme(int themeIndex)
{
	static const char *profileNames[] = {
		"amber.json", "solarized.json", "solarized-light.json", "ned.json"};
	if (themeIndex < 0 || themeIndex >= 4)
		return;

	clickedThemeIndex = themeIndex;
	clickAnimationStartTime = glfwGetTime();
	isPlayingClickAnimation = true;
	settings.switchToProfile(profileNames[themeIndex]);
}

void Welcome::render()
{
	float windowWidth, windowHeight;

	if (settings.isEmbedded)
	{
		windowWidth = ImGui::GetContentRegionAvail().x;
		windowHeight = ImGui::GetContentRegionAvail().y;
	} else
	{
		ImGuiViewport *viewport = ImGui::GetMainViewport();
		const float top = macOSTitlebarInset();
		ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + top));
		ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - top));

		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::Begin("##WelcomeScreen",
					 nullptr,
					 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
						 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

		windowWidth = ImGui::GetWindowWidth();
		windowHeight = ImGui::GetWindowHeight();
	}

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

			// Use theme text color
			auto &text = settings.settings["themes"][settings.settings.value(
				"theme", std::string("default"))]["text"];
			ImVec4 textColor = colorFromJson(text);
			ImGui::TextColored(textColor, "%s", title);

			ImGui::SetWindowFontScale(1.0f);
			ImGui::PopFont();
		}

		// Button below title - positioned right of center (bigger button)
		float buttonY = titleY + 70.0f;
		float buttonWidth = 280.0f;
		float buttonHeight = 60.0f;
		float buttonX = contentStartX;

		ImGui::SetCursorPos(ImVec2(buttonX, buttonY));
		// Get background color and make it 10% darker
		ImVec4 bgColor = colorFromJson(settings.settings["backgroundColor"]);
		ImVec4 buttonBgColor =
			ImVec4(bgColor.x * 0.9f, bgColor.y * 0.9f, bgColor.z * 0.9f, 0.8f);

		// Get font color for border
		auto &text = settings.settings["themes"][settings.settings.value(
			"theme", std::string("default"))]["text"];
		ImVec4 fontColor = colorFromJson(text);

		// Create hover and active states (20% darker for hover)
		ImVec4 hoverColor =
			ImVec4(bgColor.x * 0.8f, bgColor.y * 0.8f, bgColor.z * 0.8f, 0.9f);
		ImVec4 activeColor =
			ImVec4(bgColor.x * 0.7f, bgColor.y * 0.7f, bgColor.z * 0.7f, 1.0f);

		// Create button and check if it's hovered to set border color
		ImGui::PushStyleColor(ImGuiCol_Button, buttonBgColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

		{ // Scope for button text scaling
			ImFont *currentFont = ImGui::GetIO().Fonts->Fonts[0];
			float baseFontSize = currentFont->LegacySize;
			float desiredFontSize = 26.0f;
			float scaleFactor =
				(baseFontSize > 0) ? (desiredFontSize / baseFontSize) : 1.0f;

			ImGui::PushFont(currentFont);
			ImGui::SetWindowFontScale(scaleFactor);

			// Set border color - white if hovered, blue otherwise
			ImVec2 buttonPos = ImGui::GetCursorScreenPos();
			ImVec2 mousePos = ImGui::GetMousePos();
			bool willBeHovered =
				(mousePos.x >= buttonPos.x && mousePos.x <= buttonPos.x + buttonWidth &&
				 mousePos.y >= buttonPos.y && mousePos.y <= buttonPos.y + buttonHeight);

			if (willBeHovered)
			{
				ImGui::PushStyleColor(ImGuiCol_Border,
									  ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // White border
			} else
			{
				ImGui::PushStyleColor(ImGuiCol_Border,
									  ImVec4(0.0f, 0.48f, 1.0f, 1.0f)); // Blue border
			}

			if (ImGui::Button("Open Folder", ImVec2(buttonWidth, buttonHeight)))
			{
				std::cout << "\033[32mMain:\033[0m Welcome screen - Open Folder clicked"
						  << std::endl;
				fileExplorer.showFileDialog = true;
			}

			ImGui::PopStyleColor(); // Pop border color

			ImGui::SetWindowFontScale(1.0f);
			ImGui::PopFont();
		}

		ImGui::PopStyleVar(2);
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
										  "CMD+: Line Jump",
										  "CMD+F Find",
										  "CMD+/ Show this window"};

				// Two-column layout
				constexpr int keybindCount = 5;
				float colSpacing = 200.0f;
				for (int i = 0; i < 3; i++)
				{
					// Get theme text color for keybinds (slightly dimmed)
					auto &text = settings.settings["themes"][settings.settings.value(
						"theme", std::string("default"))]["text"];
					ImVec4 textColor = colorFromJson(text);
					ImVec4 keybindColor = ImVec4(textColor.x * 0.8f,
												 textColor.y * 0.8f,
												 textColor.z * 0.8f,
												 textColor.w);

					const int left = i * 2;
					if (left >= keybindCount)
						break;

					ImGui::SetCursorPos(ImVec2(keybindsX, keybindsY + i * 22.0f));
					ImGui::TextColored(keybindColor, "%s", keybinds[left]);

					const int right = left + 1;
					if (right < keybindCount)
					{
						ImGui::SetCursorPos(
							ImVec2(keybindsX + colSpacing, keybindsY + i * 22.0f));
						ImGui::TextColored(keybindColor, "%s", keybinds[right]);
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

			// Use theme text color
			auto &text = settings.settings["themes"][settings.settings.value(
				"theme", std::string("default"))]["text"];
			ImVec4 textColor = colorFromJson(text);
			ImGui::TextColored(textColor, "%s", title);

			ImGui::SetWindowFontScale(1.0f);
			ImGui::PopFont();
		}
		currentY += 60.0f;

		// Button - centered (bigger in vertical layout too)
		float buttonWidth = std::min(320.0f, windowWidth * 0.8f);
		float buttonHeight = 55.0f;

		ImGui::SetCursorPos(ImVec2((windowWidth - buttonWidth) * 0.5f, currentY));
		// Get background color and make it 10% darker
		ImVec4 bgColor = colorFromJson(settings.settings["backgroundColor"]);
		ImVec4 buttonBgColor =
			ImVec4(bgColor.x * 0.9f, bgColor.y * 0.9f, bgColor.z * 0.9f, 0.8f);

		// Get font color for border
		auto &text = settings.settings["themes"][settings.settings.value(
			"theme", std::string("default"))]["text"];
		ImVec4 fontColor = colorFromJson(text);

		// Create hover and active states (20% darker for hover)
		ImVec4 hoverColor =
			ImVec4(bgColor.x * 0.8f, bgColor.y * 0.8f, bgColor.z * 0.8f, 0.9f);
		ImVec4 activeColor =
			ImVec4(bgColor.x * 0.7f, bgColor.y * 0.7f, bgColor.z * 0.7f, 1.0f);

		// Create button and check if it's hovered to set border color
		ImGui::PushStyleColor(ImGuiCol_Button, buttonBgColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

		{ // Scope for button text scaling
			ImFont *currentFont = ImGui::GetIO().Fonts->Fonts[0];
			float baseFontSize = currentFont->LegacySize;
			float desiredFontSize = 24.0f;
			float scaleFactor =
				(baseFontSize > 0) ? (desiredFontSize / baseFontSize) : 1.0f;

			ImGui::PushFont(currentFont);
			ImGui::SetWindowFontScale(scaleFactor);

			// Set border color - blue if hovered, font color otherwise
			ImVec2 buttonPos = ImGui::GetCursorScreenPos();
			ImVec2 mousePos = ImGui::GetMousePos();
			bool willBeHovered =
				(mousePos.x >= buttonPos.x && mousePos.x <= buttonPos.x + buttonWidth &&
				 mousePos.y >= buttonPos.y && mousePos.y <= buttonPos.y + buttonHeight);

			if (willBeHovered)
			{
				ImGui::PushStyleColor(ImGuiCol_Border,
									  ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // White border
			} else
			{
				ImGui::PushStyleColor(ImGuiCol_Border,
									  ImVec4(0.0f, 0.48f, 1.0f, 1.0f)); // Blue border
			}

			if (ImGui::Button("Open Folder", ImVec2(buttonWidth, buttonHeight)))
			{
				std::cout << "\033[32mMain:\033[0m Welcome screen - Open Folder clicked"
						  << std::endl;
				fileExplorer.showFileDialog = true;
			}

			ImGui::PopStyleColor(); // Pop border color

			ImGui::SetWindowFontScale(1.0f);
			ImGui::PopFont();
		}

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);

		currentY += buttonHeight + 40.0f;
	}

	// Render welcome image grid below the main content (after both layouts)
	renderWelcomeImageGrid(windowWidth, windowHeight, currentY);

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

		// Make GitHub link clickable - use theme text color
		auto &text = settings.settings["themes"][settings.settings.value(
			"theme", std::string("default"))]["text"];
		ImVec4 textColor = colorFromJson(text);
		ImGui::PushStyleColor(ImGuiCol_Text, textColor);
		if (ImGui::Selectable(
				github,
				false,
				ImGuiSelectableFlags_None,
				ImVec2(githubWidth, ImGui::GetTextLineHeight() * scaleFactor)))
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

		// Add hover effect with theme background color
		if (ImGui::IsItemHovered())
		{
			ImVec4 bgColor = colorFromJson(settings.settings["backgroundColor"]);
			bgColor.w = 1.0f; // Force full opacity
			ImGui::PushStyleColor(ImGuiCol_PopupBg, bgColor);
			ImGui::SetTooltip("Click to open GitHub repository");
			ImGui::PopStyleColor();
		}

		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopFont();
	} // End scope for github link scaling

	// Must pair Begin("##WelcomeScreen") above (standalone only).
	if (!settings.isEmbedded)
	{
		ImGui::End();
		ImGui::PopStyleVar();
	}
}
