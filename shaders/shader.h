/*
	File: shader.h
	Description: Individual shader management for loading and using GLSL shaders
*/

#ifndef SHADER_H
#define SHADER_H
#include <string>

class Shader
{
  public:
	// Constructor and destructor
	Shader();
	~Shader();

	// Load and compile shader functions
	bool loadShader(const std::string &vertexShaderPath,
					const std::string &fragmentShaderPath);
	void useShader();

	// Utility functions
	void setFloat(const std::string &name, float value);
	void setInt(const std::string &name, int value);
	void setMatrix4fv(const std::string &name, const float *matrix);

	// Access to shader program (for ShaderManager)
	unsigned int getShaderProgram() const { return shaderProgram; }

  private:
	unsigned int shaderProgram;
};
#endif