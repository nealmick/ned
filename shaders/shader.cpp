/*
	File: shader.cpp
	Description: Apply shaders to IMGUI output for visual effects
*/

#include "shaders/shader.h"
#include <GL/glew.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <set>

Shader::Shader() { shaderProgram = 0; }

// Add static set to track warned uniforms
static std::set<std::string> warnedUniforms;

Shader::~Shader()
{
	if (shaderProgram != 0)
	{
		glDeleteProgram(shaderProgram);
	}
}

bool Shader::loadShader(const std::string &vertexShaderRelativePath,
						const std::string &fragmentShaderRelativePath)
{
	std::string finalVertexShaderPath;
	std::string finalFragmentShaderPath;

	const std::string debian_package_base_path = "/usr/share/Ned/";

	// --- Resolve Vertex Shader Path ---
	if (std::filesystem::exists(vertexShaderRelativePath))
	{
		finalVertexShaderPath = vertexShaderRelativePath;
		// std::cout << "[Shader] Found vertex shader at primary path: " << finalVertexShaderPath <<
		// std::endl;
	} else
	{
		std::string packagedPath = debian_package_base_path + vertexShaderRelativePath;
		if (std::filesystem::exists(packagedPath))
		{
			finalVertexShaderPath = packagedPath;
			// std::cout << "[Shader] Found vertex shader at Debian package path: " <<
			// finalVertexShaderPath << std::endl;
		} else
		{
			std::cerr << "🔴 ERROR: Cannot find vertex shader. Tried:\n"
					  << "  1. Relative/Dev path: " << vertexShaderRelativePath << "\n"
					  << "  2. Debian package path: " << packagedPath << std::endl;
			return false;
		}
	}

	// --- Resolve Fragment Shader Path ---
	if (std::filesystem::exists(fragmentShaderRelativePath))
	{
		finalFragmentShaderPath = fragmentShaderRelativePath;
		// std::cout << "[Shader] Found fragment shader at primary path: " <<
		// finalFragmentShaderPath << std::endl;
	} else
	{
		std::string packagedPath = debian_package_base_path + fragmentShaderRelativePath;
		if (std::filesystem::exists(packagedPath))
		{
			finalFragmentShaderPath = packagedPath;
			// std::cout << "[Shader] Found fragment shader at Debian package path: " <<
			// finalFragmentShaderPath << std::endl;
		} else
		{
			std::cerr << "🔴 ERROR: Cannot find fragment shader. Tried:\n"
					  << "  1. Relative/Dev path: " << fragmentShaderRelativePath << "\n"
					  << "  2. Debian package path: " << packagedPath << std::endl;
			return false;
		}
	}

	// std::cout << "Attempting to load vertex shader from: " << finalVertexShaderPath << std::endl;
	// std::cout << "Attempting to load fragment shader from: " << finalFragmentShaderPath <<
	// std::endl;

	// Read vertex shader
	std::string vertexShaderCode;
	std::ifstream vertexShaderFile(finalVertexShaderPath);
	if (!vertexShaderFile.is_open())
	{
		// This should ideally not be reached if std::filesystem::exists passed, but good for
		// robustness
		std::cerr << "🔴 ERROR: Cannot open resolved vertex shader file: " << finalVertexShaderPath
				  << std::endl;
		return false;
	}
	std::stringstream vertexShaderStream;
	vertexShaderStream << vertexShaderFile.rdbuf();
	vertexShaderCode = vertexShaderStream.str();
	vertexShaderFile.close();

	// Read fragment shader
	std::string fragmentShaderCode;
	std::ifstream fragmentShaderFile(finalFragmentShaderPath);
	if (!fragmentShaderFile.is_open())
	{
		// This should ideally not be reached if std::filesystem::exists passed
		std::cerr << "🔴 ERROR: Cannot open resolved fragment shader file: "
				  << finalFragmentShaderPath << std::endl;
		return false;
	}
	std::stringstream fragmentShaderStream;
	fragmentShaderStream << fragmentShaderFile.rdbuf();
	fragmentShaderCode = fragmentShaderStream.str();
	fragmentShaderFile.close();

	// Check if shaders are empty
	if (vertexShaderCode.empty())
	{
		std::cerr << "🔴 ERROR: Vertex shader is empty (from " << finalVertexShaderPath << ")!"
				  << std::endl;
		return false;
	}
	if (fragmentShaderCode.empty())
	{
		std::cerr << "🔴 ERROR: Fragment shader is empty (from " << finalFragmentShaderPath << ")!"
				  << std::endl;
		return false;
	}

	// Compile vertex shader
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	const char *vertexShaderCodePtr = vertexShaderCode.c_str();
	glShaderSource(vertexShader, 1, &vertexShaderCodePtr, NULL);
	glCompileShader(vertexShader);

	int success = 0;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cerr << "🔴 ERROR: Vertex shader compilation failed (from " << finalVertexShaderPath
				  << ")\n"
				  << infoLog << std::endl;
		return false;
	}

	// Compile fragment shader
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	const char *fragmentShaderCodePtr = fragmentShaderCode.c_str();
	glShaderSource(fragmentShader, 1, &fragmentShaderCodePtr, NULL);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cerr << "🔴 ERROR: Fragment shader compilation failed (from "
				  << finalFragmentShaderPath << ")\n"
				  << infoLog << std::endl;
		glDeleteShader(vertexShader);
		return false;
	}

	// Create and link program
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cerr << "🔴 SHADER LINK ERROR: " << infoLog << std::endl;
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		return false;
	}

	// Debug output for uniforms (optional)
	GLint numUniforms = 0;
	glGetProgramiv(shaderProgram, GL_ACTIVE_UNIFORMS, &numUniforms);
	for (GLint i = 0; i < numUniforms; i++)
	{
		char uniformName[256];
		GLsizei length;
		GLint size;
		GLenum type;
		glGetActiveUniform(
			shaderProgram, i, sizeof(uniformName), &length, &size, &type, uniformName);
		// GLint location = glGetUniformLocation(shaderProgram, uniformName);
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// GLint screenTexLoc = glGetUniformLocation(shaderProgram, "screenTexture"); // Optional check

	return true;
}
void Shader::useShader()
{
	glUseProgram(shaderProgram);

	// Remove this check entirely - not all shaders need 'screenTexture'
	// GLint textureLocation = glGetUniformLocation(shaderProgram, "screenTexture");
	// if (textureLocation == -1) {
	//     std::cerr << "🔴 Failed to find 'screenTexture' uniform" << std::endl;
	// }
}

void Shader::setFloat(const std::string &name, float value)
{
	GLint location = glGetUniformLocation(shaderProgram, name.c_str());
	if (location == -1)
	{
		// Only warn if we haven't warned about this uniform before
		if (warnedUniforms.find(name) == warnedUniforms.end()) {
			std::cerr << "Warning: Uniform '" << name << "' not found in shader program" << std::endl;
			warnedUniforms.insert(name);
		}
		return;
	}
	glUniform1f(location, value);
}
void Shader::setMatrix4fv(const std::string &name, const float *matrix)
{
	GLint location = glGetUniformLocation(shaderProgram, name.c_str());
	if (location == -1)
	{
		std::cerr << "🔴 Warning: Matrix uniform '" << name << "' not found" << std::endl;
		return;
	}
	glUniformMatrix4fv(location, 1, GL_FALSE, matrix);
}
void Shader::setInt(const std::string &name, int value)
{
	GLint location = glGetUniformLocation(shaderProgram, name.c_str());
	if (location == -1)
	{
		std::cerr << "🔴 Warning: Uniform '" << name << "' not found in shader program"
				  << std::endl;
		return;
	}
	glUniform1i(location, value);
}
