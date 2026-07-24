/*
	File: shader_manager.cpp
	Description: Implementation of shader manager for organizing multiple shaders
*/

#include "shaders/shader_manager.h"
#include "util/settings.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

ShaderManager::ShaderManager(Settings &settings,
							 FramebufferState &fb,
							 AccumulationBuffers &accum,
							 ShaderQuad &quad)
	: settings(settings), fb(fb), accum(accum), quad(quad)
{
}

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

void ShaderManager::initializeFramebuffers(int width, int height)
{
	auto initFB = [](FramebufferState &fb, int w, int h) {
		if (fb.initialized && w == fb.lastDisplayW && h == fb.lastDisplayH)
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

		fb.lastDisplayW = w;
		fb.lastDisplayH = h;
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

void ShaderManager::cleanupFramebuffers()
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

void ShaderManager::renderWithEffects(GLFWwindow *window)
{
	if (shaderEnabled)
		renderBurnInPass();
	renderCRTEffects(window);
}

void ShaderManager::renderBurnInPass()
{
	int prev = accum.swap ? 1 : 0;
	int curr = accum.swap ? 0 : 1;

	glBindFramebuffer(GL_FRAMEBUFFER, accum.accum[curr].framebuffer);
	burnInShader.useShader();
	burnInShader.setInt("currentFrame", 0);
	burnInShader.setInt("previousFrame", 1);
	burnInShader.setFloat("decay", settings.settings["burnin_intensity"]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fb.renderTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, accum.accum[prev].renderTexture);

	glClear(GL_COLOR_BUFFER_BIT);
	glBindVertexArray(quad.VAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	accum.swap = !accum.swap;
}

void ShaderManager::renderCRTEffects(GLFWwindow *window)
{
	int display_w = 0, display_h = 0;
	if (window)
		glfwGetFramebufferSize(window, &display_w, &display_h);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	crtShader.useShader();

	crtShader.setInt("screenTexture", 0);
	crtShader.setFloat("u_effects_enabled", shaderEnabled ? 1.0f : 0.0f);
	crtShader.setFloat("u_scanline_intensity", settings.settings["scanline_intensity"]);
	crtShader.setFloat("u_vignet_intensity", settings.settings["vignet_intensity"]);
	crtShader.setFloat("u_bloom_intensity", settings.settings["bloom_intensity"]);
	crtShader.setFloat("u_static_intensity", settings.settings["static_intensity"]);
	crtShader.setFloat("u_colorshift_intensity",
					   settings.settings["colorshift_intensity"]);
	crtShader.setFloat("u_jitter_intensity",
					   settings.settings["jitter_intensity"].get<float>());
	crtShader.setFloat("u_curvature_intensity",
					   settings.settings["curvature_intensity"].get<float>());
	crtShader.setFloat("u_pixelation_intensity",
					   settings.settings["pixelation_intensity"].get<float>());
	crtShader.setFloat("u_pixel_width", settings.settings["pixel_width"].get<float>());

	GLint timeLocation = glGetUniformLocation(crtShader.getShaderProgram(), "time");
	GLint resolutionLocation =
		glGetUniformLocation(crtShader.getShaderProgram(), "resolution");
	if (timeLocation != -1)
		glUniform1f(timeLocation, static_cast<float>(glfwGetTime()));
	if (resolutionLocation != -1)
		glUniform2f(resolutionLocation,
					static_cast<float>(display_w),
					static_cast<float>(display_h));

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
