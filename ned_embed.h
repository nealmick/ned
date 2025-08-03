/*
File: ned_embed.h
Description: Embeddable NED editor wrapper for use within other ImGui projects.
This provides the full NED editor functionality without window management.
*/

#pragma once

#include <imgui.h>
#include <string>

// Forward declarations
class ImFont;
class Settings;
class Splitter;
class WindowResize;
class ShaderManager;
class Render;
class ShaderQuad;
struct FramebufferState;
struct AccumulationBuffers;

// Embeddable NED Editor class
class NedEmbed
{
  public:
	NedEmbed();
	~NedEmbed();

	// Initialize the embeddable editor (everything except GLFW window)
	bool initialize();

	// Render the editor as an ImGui window/child
	// This should be called within an ImGui window or child window
	void render(float width, float height);

	// Handle input (call this before rendering if needed)
	void handleInput();

	// Handle font reloading (call this before ImGui frame begins)
	void handleFontReload();

	// Cleanup resources
	void cleanup();

	// Configuration methods
	void setShowSidebar(bool show);
	void setShowAgentPane(bool show);
	void setShowLineNumbers(bool show);
	void setShowWelcome(bool show);

	bool getShowSidebar() const { return showSidebar; }
	bool getShowAgentPane() const { return showAgentPane; }
	bool getShowLineNumbers() const { return showLineNumbers; }

  private:
	// Core components (reused from main NED)
	Settings *settings;
	Splitter *splitter;
	WindowResize *windowResize;

	// Configuration
	bool showSidebar;
	bool showAgentPane;
	bool showLineNumbers;
	bool showWelcome;
	bool isEmbedded;
	bool initialized;

	// Internal rendering methods
	void renderEditor(float editorWidth);
	void renderFileExplorer(float explorerWidth);
	void renderAgentPane(float agentWidth);
	void renderSplitter(float padding, float availableWidth);

	// Initialize core components (without GLFW window)
	bool initializeComponents();
	void cleanupComponents();
};