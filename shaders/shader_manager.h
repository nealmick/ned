/*
	File: shader_manager.h
	Description: Manages multiple shaders and their rendering pipeline
*/

#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#include "shaders/shader.h"
#include "shaders/shader_types.h"

class ShaderManager
{
  public:
	ShaderManager();
	~ShaderManager();

	// Shader management
	bool initializeShaders();

	// Framebuffer management
	void initializeFramebuffers(int width,
								int height,
								FramebufferState &fb,
								AccumulationBuffers &accum);
	void cleanupFramebuffers(FramebufferState &fb, AccumulationBuffers &accum);

	// Rendering pipeline
	void renderWithEffects(int display_w,
						   int display_h,
						   double currentTime,
						   const FramebufferState &fb,
						   AccumulationBuffers &accum,
						   const ShaderQuad &quad,
						   class Settings &gSettings);

	// State management
	void setShaderEnabled(bool enabled);
	bool isShaderEnabled() const;

  private:
	Shader crtShader;
	Shader burnInShader;
	bool shaderEnabled = false;

	// Internal rendering methods
	void renderBurnInPass(const FramebufferState &fb,
						  AccumulationBuffers &accum,
						  const ShaderQuad &quad,
						  class Settings &gSettings);
	void renderCRTEffects(int display_w,
						  int display_h,
						  double currentTime,
						  const FramebufferState &fb,
						  const AccumulationBuffers &accum,
						  const ShaderQuad &quad,
						  class Settings &gSettings);
};

#endif // SHADER_MANAGER_H