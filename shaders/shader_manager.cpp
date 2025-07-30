/*
	File: shader_manager.cpp
	Description: Implementation of shader manager for organizing multiple shaders
*/

#include "shaders/shader_manager.h"
#include "util/settings.h"
#include <GL/glew.h>
#include <iostream>

ShaderManager::ShaderManager() {}

ShaderManager::~ShaderManager() {}

bool ShaderManager::initializeShaders()
{
	// Load CRT shader (main effects)
	if (!crtShader.loadShader("shaders/vertex.glsl", "shaders/fragment.glsl"))
	{
		std::cerr << "🔴 Failed to load CRT shader" << std::endl;
		return false;
	}

	// Load burn-in shader (accumulation effect)
	if (!burnInShader.loadShader("shaders/vertex.glsl", "shaders/burn_in.frag"))
	{
		std::cerr << "🔴 Failed to load burn-in shader" << std::endl;
		return false;
	}

	return true;
}

void ShaderManager::initializeFramebuffers(int width,
										   int height,
										   FramebufferState &fb,
										   AccumulationBuffers &accum)
{
	auto initFB = [](FramebufferState &fb, int w, int h) {
		if (fb.initialized && w == fb.last_display_w && h == fb.last_display_h)
			return;

		// Delete old resources if they exist
		if (fb.initialized)
		{
			glDeleteFramebuffers(1, &fb.framebuffer);
			glDeleteTextures(1, &fb.renderTexture);
			glDeleteRenderbuffers(1, &fb.rbo);
		}

		// Create new framebuffer
		glGenFramebuffers(1, &fb.framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, fb.framebuffer);

		// Create texture
		glGenTextures(1, &fb.renderTexture);
		glBindTexture(GL_TEXTURE_2D, fb.renderTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb.renderTexture, 0);

		// Create renderbuffer
		glGenRenderbuffers(1, &fb.rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, fb.rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
		glFramebufferRenderbuffer(
			GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fb.rbo);

		// Check completeness
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cerr << "🔴 Framebuffer not complete!" << std::endl;
		}

		fb.last_display_w = w;
		fb.last_display_h = h;
		fb.initialized = true;

		glBindFramebuffer(GL_FRAMEBUFFER, fb.framebuffer);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		if (fb.renderTexture)
		{
			glBindTexture(GL_TEXTURE_2D, fb.renderTexture);
			glTexImage2D(
				GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		}
	};

	initFB(fb, width, height);
	initFB(accum.accum[0], width, height);
	initFB(accum.accum[1], width, height);

	// Add debug checks after initialization
	glBindFramebuffer(GL_FRAMEBUFFER, accum.accum[0].framebuffer);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cerr << "🔴 Accumulation buffer 0 incomplete!" << std::endl;
	}
}

void ShaderManager::cleanupFramebuffers(FramebufferState &fb, AccumulationBuffers &accum)
{
	if (fb.initialized)
	{
		glDeleteFramebuffers(1, &fb.framebuffer);
		glDeleteTextures(1, &fb.renderTexture);
		glDeleteRenderbuffers(1, &fb.rbo);
		fb.initialized = false;
	}

	if (accum.accum[0].initialized)
	{
		glDeleteFramebuffers(1, &accum.accum[0].framebuffer);
		glDeleteTextures(1, &accum.accum[0].renderTexture);
		accum.accum[0].initialized = false;
	}
	if (accum.accum[1].initialized)
	{
		glDeleteFramebuffers(1, &accum.accum[1].framebuffer);
		glDeleteTextures(1, &accum.accum[1].renderTexture);
		accum.accum[1].initialized = false;
	}
}

void ShaderManager::renderWithEffects(int display_w,
									  int display_h,
									  double currentTime,
									  const FramebufferState &fb,
									  AccumulationBuffers &accum,
									  const ShaderQuad &quad,
									  class Settings &gSettings)
{
	if (shaderEnabled)
	{
		renderBurnInPass(fb, accum, quad, gSettings);
	}
	renderCRTEffects(display_w, display_h, currentTime, fb, accum, quad, gSettings);
}

void ShaderManager::renderBurnInPass(const FramebufferState &fb,
									 AccumulationBuffers &accum,
									 const ShaderQuad &quad,
									 class Settings &gSettings)
{
	int prev = accum.swap ? 1 : 0;
	int curr = accum.swap ? 0 : 1;

	glBindFramebuffer(GL_FRAMEBUFFER, accum.accum[curr].framebuffer);
	burnInShader.useShader();
	burnInShader.setInt("currentFrame", 0);
	burnInShader.setInt("previousFrame", 1);
	burnInShader.setFloat("decay", gSettings.getSettings()["burnin_intensity"]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fb.renderTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, accum.accum[prev].renderTexture);

	glClear(GL_COLOR_BUFFER_BIT);
	glBindVertexArray(quad.VAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	accum.swap = !accum.swap;
}

void ShaderManager::renderCRTEffects(int display_w,
									 int display_h,
									 double currentTime,
									 const FramebufferState &fb,
									 const AccumulationBuffers &accum,
									 const ShaderQuad &quad,
									 class Settings &gSettings)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	crtShader.useShader();

	crtShader.setInt("screenTexture", 0);
	if (shaderEnabled)
	{
		crtShader.setFloat("u_effects_enabled", 1.0f);
	} else
	{
		crtShader.setFloat("u_effects_enabled", 0.0f);
	}
	crtShader.setFloat("u_scanline_intensity",
					   gSettings.getSettings()["scanline_intensity"]);
	crtShader.setFloat("u_vignet_intensity", gSettings.getSettings()["vignet_intensity"]);
	crtShader.setFloat("u_bloom_intensity", gSettings.getSettings()["bloom_intensity"]);
	crtShader.setFloat("u_static_intensity", gSettings.getSettings()["static_intensity"]);
	crtShader.setFloat("u_colorshift_intensity",
					   gSettings.getSettings()["colorshift_intensity"]);
	crtShader.setFloat("u_jitter_intensity",
					   gSettings.getSettings()["jitter_intensity"].get<float>());
	crtShader.setFloat("u_curvature_intensity",
					   gSettings.getSettings()["curvature_intensity"].get<float>());
	crtShader.setFloat("u_pixelation_intensity",
					   gSettings.getSettings()["pixelation_intensity"].get<float>());
	crtShader.setFloat("u_pixel_width",
					   gSettings.getSettings()["pixel_width"].get<float>());

	GLint timeLocation = glGetUniformLocation(crtShader.getShaderProgram(), "time");
	GLint resolutionLocation =
		glGetUniformLocation(crtShader.getShaderProgram(), "resolution");
	if (timeLocation != -1)
		glUniform1f(timeLocation, currentTime);
	if (resolutionLocation != -1)
		glUniform2f(resolutionLocation, display_w, display_h);

	glActiveTexture(GL_TEXTURE0);
	if (shaderEnabled)
	{
		int curr = accum.swap ? 0 : 1;
		glBindTexture(GL_TEXTURE_2D, accum.accum[curr].renderTexture);
	} else
	{
		glBindTexture(GL_TEXTURE_2D, fb.renderTexture);
	}

	glBindVertexArray(quad.VAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void ShaderManager::setShaderEnabled(bool enabled) { shaderEnabled = enabled; }

bool ShaderManager::isShaderEnabled() const { return shaderEnabled; }