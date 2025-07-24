/*
	File: shader.cpp
	Description: Individual shader management for loading and using GLSL shaders
*/

#include "shaders/shader.h"
#include <GL/glew.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <unistd.h>

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
	} else
	{
		std::string packagedPath = debian_package_base_path + vertexShaderRelativePath;
		if (std::filesystem::exists(packagedPath))
		{
			finalVertexShaderPath = packagedPath;
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
	} else
	{
		std::string packagedPath = debian_package_base_path + fragmentShaderRelativePath;
		if (std::filesystem::exists(packagedPath))
		{
			finalFragmentShaderPath = packagedPath;
		} else
		{
			std::cerr << "🔴 ERROR: Cannot find fragment shader. Tried:\n"
					  << "  1. Relative/Dev path: " << fragmentShaderRelativePath << "\n"
					  << "  2. Debian package path: " << packagedPath << std::endl;
			return false;
		}
	}

	// Read vertex shader
	std::string vertexShaderCode;
	std::ifstream vertexShaderFile(finalVertexShaderPath);
	if (!vertexShaderFile.is_open())
	{
		std::cerr << "🔴 ERROR: Cannot open resolved vertex shader file: "
				  << finalVertexShaderPath << std::endl;
		return false;
	}
	std::stringstream vertexShaderStream;
	vertexShaderStream << vertexShaderFile.rdbuf();
	vertexShaderCode = vertexShaderStream.str();

	// Read fragment shader
	std::string fragmentShaderCode;
	std::ifstream fragmentShaderFile(finalFragmentShaderPath);
	if (!fragmentShaderFile.is_open())
	{
		std::cerr << "🔴 ERROR: Cannot open resolved fragment shader file: "
				  << finalFragmentShaderPath << std::endl;
		return false;
	}
	std::stringstream fragmentShaderStream;
	fragmentShaderStream << fragmentShaderFile.rdbuf();
	fragmentShaderCode = fragmentShaderStream.str();

	// Compile vertex shader
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	const char *vertexShaderSource = vertexShaderCode.c_str();
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	// Check vertex shader compilation
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cerr << "🔴 ERROR: Vertex shader compilation failed: " << infoLog
				  << std::endl;
		glDeleteShader(vertexShader);
		return false;
	}

	// Compile fragment shader
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	const char *fragmentShaderSource = fragmentShaderCode.c_str();
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	// Check fragment shader compilation
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cerr << "🔴 ERROR: Fragment shader compilation failed: " << infoLog
				  << std::endl;
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		return false;
	}

	// Create shader program
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	// Check linking
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cerr << "🔴 ERROR: Shader program linking failed: " << infoLog << std::endl;
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		glDeleteProgram(shaderProgram);
		shaderProgram = 0;
		return false;
	}

	// Clean up shaders (they're now linked into the program)
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return true;
}

void Shader::useShader() { glUseProgram(shaderProgram); }

void Shader::setFloat(const std::string &name, float value)
{
	GLint location = glGetUniformLocation(shaderProgram, name.c_str());
	if (location != -1)
	{
		glUniform1f(location, value);
	} else
	{
		// Only warn once per uniform name per session
		if (warnedUniforms.find(name) == warnedUniforms.end())
		{
			std::cerr << "⚠️  Warning: Uniform '" << name
					  << "' not found in shader program" << std::endl;
			warnedUniforms.insert(name);
		}
	}
}

void Shader::setInt(const std::string &name, int value)
{
	GLint location = glGetUniformLocation(shaderProgram, name.c_str());
	if (location != -1)
	{
		glUniform1i(location, value);
	} else
	{
		// Only warn once per uniform name per session
		if (warnedUniforms.find(name) == warnedUniforms.end())
		{
			std::cerr << "⚠️  Warning: Uniform '" << name
					  << "' not found in shader program" << std::endl;
			warnedUniforms.insert(name);
		}
	}
}

void Shader::setMatrix4fv(const std::string &name, const float *matrix)
{
	GLint location = glGetUniformLocation(shaderProgram, name.c_str());
	if (location != -1)
	{
		glUniformMatrix4fv(location, 1, GL_FALSE, matrix);
	} else
	{
		// Only warn once per uniform name per session
		if (warnedUniforms.find(name) == warnedUniforms.end())
		{
			std::cerr << "⚠️  Warning: Uniform '" << name
					  << "' not found in shader program" << std::endl;
			warnedUniforms.insert(name);
		}
	}
}
