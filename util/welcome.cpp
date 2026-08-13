#include "welcome.h"
#include "../files/files.h"
#include "macos_window.h"
#include "settings.h"
#include <algorithm>
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

ImVec4 themeTextColor(const Settings &settings)
{
	const auto &text = settings.settings["themes"][settings.settings.value(
		"theme", std::string("default"))]["text"];
	return colorFromJson(text);
}

bool drawOpenFolderButton(const Settings &settings, const ImVec2 &size)
{
	const ImVec4 bg = colorFromJson(settings.settings["backgroundColor"]);
	ImGui::PushStyleColor(ImGuiCol_Button,
						  ImVec4(bg.x * 0.9f, bg.y * 0.9f, bg.z * 0.9f, 0.8f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
						  ImVec4(bg.x * 0.8f, bg.y * 0.8f, bg.z * 0.8f, 0.9f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,
						  ImVec4(bg.x * 0.7f, bg.y * 0.7f, bg.z * 0.7f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, ImGui::GetFontSize() * 0.3f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

	const ImVec2 pos = ImGui::GetCursorScreenPos();
	const ImVec2 mouse = ImGui::GetMousePos();
	const bool hovered = mouse.x >= pos.x && mouse.x <= pos.x + size.x &&
						 mouse.y >= pos.y && mouse.y <= pos.y + size.y;
	ImGui::PushStyleColor(ImGuiCol_Border,
						  hovered ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
								  : ImVec4(0.0f, 0.48f, 1.0f, 1.0f));

	const bool clicked = ImGui::Button("Open Folder", size);
	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar(2);
	return clicked;
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
	const float fs = ImGui::GetFontSize();
	if (windowWidth < fs * 30.0f || !loadWelcomeImages())
		return;

	const float remainingHeight = windowHeight - currentY - fs * 5.0f;
	if (remainingHeight < fs * 7.5f)
		return;

	const float effectiveWidth = std::min(windowWidth, fs * 55.0f);
	float imageSize = std::max(fs * 4.0f, std::min(fs * 8.0f, effectiveWidth * 0.1f));
	if (effectiveWidth > fs * 50.0f)
		imageSize *= 1.1f;

	const float spacing = fs;
	float totalGridWidth = (imageSize * 4.0f) + (spacing * 3.0f);
	if (totalGridWidth > windowWidth - fs * 2.0f)
	{
		imageSize = (windowWidth - fs * 2.0f - spacing * 3.0f) / 4.0f;
		if (imageSize < fs * 4.0f)
			return;
		totalGridWidth = (imageSize * 4.0f) + (spacing * 3.0f);
	}

	const float startX = (windowWidth - totalGridWidth) * 0.5f;
	const float rounding = fs * 0.6f;
	currentY += fs * 1.5f;

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
								   ImVec2(1, 1),
								   IM_COL32_WHITE,
								   rounding);

		const bool isHovered = ImGui::IsItemHovered();
		if (isHovered)
		{
			draw_list->AddRectFilled(rectMin, rectMax, IM_COL32(0, 123, 255, 76), rounding);
		}

		draw_list->AddRect(
			rectMin, rectMax, IM_COL32(128, 128, 128, 80), rounding, 0, 1.5f);

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
				rectMin, rectMax, IM_COL32(255, 255, 255, alpha), rounding, 0, 3.0f);
		}

		if (isHovered && !showClickAnimation)
		{
			draw_list->AddRect(
				rectMin, rectMax, IM_COL32(0, 123, 255, 200), rounding, 0, 2.5f);

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

	ImFont *font = ImGui::GetFont();
	const float body = ImGui::GetStyle().FontSizeBase;
	const float fs = ImGui::GetFontSize();
	const ImVec4 textColor = themeTextColor(settings);

	const float minTop = fs;
	const float maxTop = fs * 4.0f;
	const float topMargin =
		minTop + (maxTop - minTop) *
					 std::min(1.0f, (windowHeight - fs * 20.0f) / (fs * 20.0f));
	float currentY = topMargin;

	const bool useSideBySide = windowWidth > fs * 40.0f && loadNedLogo();

	if (useSideBySide)
	{
		const float logoSize =
			std::min(windowWidth * 0.31f, std::min(fs * 15.5f, windowHeight * 0.49f));
		const float centerX = windowWidth * 0.5f;
		const float spacing = fs * 3.0f;
		const float logoX = centerX - spacing - logoSize;
		const float logoY = currentY + fs;
		const float contentX = centerX + spacing - fs * 5.0f;

		ImGui::SetCursorPos(ImVec2(logoX, logoY));
		ImGui::Image((ImTextureID)(intptr_t)nedLogoTexture, ImVec2(logoSize, logoSize));

		ImGui::PushFont(font, body * 3.2f);
		ImGui::SetCursorPos(ImVec2(contentX, logoY + fs));
		ImGui::TextColored(textColor, "Welcome to NED");
		const float afterTitle = ImGui::GetCursorPosY();
		ImGui::PopFont();

		ImGui::PushFont(font, body * 1.3f);
		const ImVec2 btnSize(std::max(ImGui::CalcTextSize("Open Folder").x +
										  ImGui::GetFontSize() * 3.2f,
									  ImGui::GetFontSize() * 10.8f),
							 ImGui::GetTextLineHeight() + ImGui::GetFontSize() * 0.9f);
		ImGui::SetCursorPos(
			ImVec2(contentX, afterTitle + ImGui::GetTextLineHeight() * 0.35f));
		if (drawOpenFolderButton(settings, btnSize))
		{
			std::cout << "\033[32mMain:\033[0m Welcome screen - Open Folder clicked"
					  << std::endl;
			fileExplorer.showFileDialog = true;
		}
		const float afterButton = ImGui::GetCursorPosY();
		ImGui::PopFont();

		float keybindsBottom = afterButton;
		if (windowHeight > afterButton + fs * 6.0f)
		{
			ImGui::PushFont(font, body);
			const float rowH = ImGui::GetTextLineHeightWithSpacing();
			const float colW = ImGui::CalcTextSize("CMD+: Line Jump").x + fs * 2.0f;
			const ImVec4 keybindColor(textColor.x * 0.8f,
									  textColor.y * 0.8f,
									  textColor.z * 0.8f,
									  textColor.w);
			const char *keybinds[] = {"CMD+O Open Folder",
									  "CMD+T Terminal",
									  "CMD+: Line Jump",
									  "CMD+F Find",
									  "CMD+/ Show this window"};
			const float keybindsY = afterButton + rowH * 0.6f;
			for (int i = 0; i < 3; i++)
			{
				const int left = i * 2;
				if (left >= 5)
					break;
				ImGui::SetCursorPos(ImVec2(contentX, keybindsY + i * rowH));
				ImGui::TextColored(keybindColor, "%s", keybinds[left]);
				if (left + 1 < 5)
				{
					ImGui::SetCursorPos(ImVec2(contentX + colW, keybindsY + i * rowH));
					ImGui::TextColored(keybindColor, "%s", keybinds[left + 1]);
				}
			}
			keybindsBottom = keybindsY + 3.0f * rowH;
			ImGui::PopFont();
		}

		currentY = std::max(logoY + logoSize, keybindsBottom) + fs * 2.0f;
	} else
	{
		if (loadNedLogo() && windowHeight > fs * 20.0f)
		{
			const float logoSize =
				std::min(windowWidth * 0.3f, std::min(fs * 7.5f, windowHeight * 0.2f));
			ImGui::SetCursorPos(ImVec2((windowWidth - logoSize) * 0.5f, currentY));
			ImGui::Image((ImTextureID)(intptr_t)nedLogoTexture,
						 ImVec2(logoSize, logoSize));
			currentY += logoSize + fs * 1.5f;
		}

		ImGui::PushFont(font, body * 2.0f);
		const char *title = "Welcome to NED";
		ImGui::SetCursorPos(
			ImVec2((windowWidth - ImGui::CalcTextSize(title).x) * 0.5f, currentY));
		ImGui::TextColored(textColor, "%s", title);
		currentY = ImGui::GetCursorPosY() + ImGui::GetTextLineHeight() * 0.5f;
		ImGui::PopFont();

		ImGui::PushFont(font, body * 1.2f);
		const ImVec2 btnSize(std::min(ImGui::GetFontSize() * 12.3f, windowWidth * 0.8f),
							 ImGui::GetTextLineHeight() + ImGui::GetFontSize() * 0.9f);
		ImGui::SetCursorPos(ImVec2((windowWidth - btnSize.x) * 0.5f, currentY));
		if (drawOpenFolderButton(settings, btnSize))
		{
			std::cout << "\033[32mMain:\033[0m Welcome screen - Open Folder clicked"
					  << std::endl;
			fileExplorer.showFileDialog = true;
		}
		currentY = ImGui::GetCursorPosY() + fs * 2.0f;
		ImGui::PopFont();
	}

	renderWelcomeImageGrid(windowWidth, windowHeight, currentY);

	const char *github = "github.com/nealmick/ned";
	ImGui::SetCursorPosY(windowHeight - ImGui::GetTextLineHeight() * 3.0f);
	ImGui::SetCursorPosX((windowWidth - ImGui::CalcTextSize(github).x) * 0.5f);
	ImGui::PushStyleColor(ImGuiCol_Text, textColor);
	if (ImGui::Selectable(github,
						  false,
						  ImGuiSelectableFlags_None,
						  ImVec2(ImGui::CalcTextSize(github).x,
								 ImGui::GetTextLineHeight())))
	{
#ifdef __APPLE__
		system("open https://github.com/nealmick/ned");
#elif defined(_WIN32)
		system("start https://github.com/nealmick/ned");
#else
		system("xdg-open https://github.com/nealmick/ned");
#endif
	}
	ImGui::PopStyleColor();
	if (ImGui::IsItemHovered())
	{
		ImVec4 bgColor = colorFromJson(settings.settings["backgroundColor"]);
		bgColor.w = 1.0f;
		ImGui::PushStyleColor(ImGuiCol_PopupBg, bgColor);
		ImGui::SetTooltip("Click to open GitHub repository");
		ImGui::PopStyleColor();
	}

	if (!settings.isEmbedded)
	{
		ImGui::End();
		ImGui::PopStyleVar();
	}
}
