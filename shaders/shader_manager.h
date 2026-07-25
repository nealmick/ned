/*
	File: shader_manager.h
	Description: Manages multiple shaders and their rendering pipeline
*/

#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#include "shaders/shader.h"
#include "shaders/shader_types.h"
#include "util/settings.h"

struct GLFWwindow;

class ShaderManager
{
  public:
	ShaderManager(Settings &settings,
				  FramebufferState &fb,
				  AccumulationBuffers &accum,
				  ShaderQuad &quad);

	// Shader management
	bool initializeShaders();

	// Framebuffer management
	void initializeFramebuffers(int width, int height);
	void cleanupFramebuffers();

	// Rendering pipeline (window used for framebuffer size queries)
	void renderWithEffects(GLFWwindow *window);

	// State management
	void setShaderEnabled(bool enabled);
	bool isShaderEnabled() const;

  private:
	Settings &settings;
	FramebufferState &fb;
	AccumulationBuffers &accum;
	ShaderQuad &quad;

	Shader crtShader;
	Shader burnInShader;
	bool shaderEnabled = false;

	void renderBurnInPass();
	void renderCRTEffects(GLFWwindow *window);
};

#endif // SHADER_MANAGER_H
