// shader.cpp
#include "shaders/shader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <GL/glew.h>
#include <unistd.h>  

Shader::Shader() {
    shaderProgram = 0;
}

Shader::~Shader() {
    if (shaderProgram != 0) {
        glDeleteProgram(shaderProgram);
    }
}
bool Shader::loadShader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        std::cout << "Current working directory: " << cwd << std::endl;
    }
    std::cout << "Attempting to load vertex shader from: " << vertexShaderPath << std::endl;
    std::cout << "Attempting to load fragment shader from: " << fragmentShaderPath << std::endl;

    // Read vertex shader
    std::string vertexShaderCode;
    std::ifstream vertexShaderFile(vertexShaderPath);
    if (!vertexShaderFile.is_open()) {
        std::cerr << "ERROR: Cannot open vertex shader file: " << vertexShaderPath << std::endl;
        return false;
    }
    std::stringstream vertexShaderStream;
    vertexShaderStream << vertexShaderFile.rdbuf();
    vertexShaderCode = vertexShaderStream.str();
    vertexShaderFile.close();

    // Print vertex shader code
    std::cout << "Vertex Shader Code:" << std::endl;
    std::cout << vertexShaderCode << std::endl;

    // Read fragment shader
    std::string fragmentShaderCode;
    std::ifstream fragmentShaderFile(fragmentShaderPath);
    if (!fragmentShaderFile.is_open()) {
        std::cerr << "ERROR: Cannot open fragment shader file: " << fragmentShaderPath << std::endl;
        return false;
    }
    std::stringstream fragmentShaderStream;
    fragmentShaderStream << fragmentShaderFile.rdbuf();
    fragmentShaderCode = fragmentShaderStream.str();
    fragmentShaderFile.close();

    // Print fragment shader code
    std::cout << "Fragment Shader Code:" << std::endl;
    std::cout << fragmentShaderCode << std::endl;

    // Check if shaders are empty
    if (vertexShaderCode.empty()) {
        std::cerr << "ERROR: Vertex shader is empty!" << std::endl;
        return false;
    }
    if (fragmentShaderCode.empty()) {
        std::cerr << "ERROR: Fragment shader is empty!" << std::endl;
        return false;
    }

    // Compile vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vertexShaderCodePtr = vertexShaderCode.c_str();
    glShaderSource(vertexShader, 1, &vertexShaderCodePtr, NULL);
    glCompileShader(vertexShader);

    // Check vertex shader compilation
    int success = 0;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Vertex shader compilation failed\n" << infoLog << std::endl;
        return false;
    }

    // Compile fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragmentShaderCodePtr = fragmentShaderCode.c_str();
    glShaderSource(fragmentShader, 1, &fragmentShaderCodePtr, NULL);
    glCompileShader(fragmentShader);

    // Check fragment shader compilation
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Fragment shader compilation failed\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);  // Clean up vertex shader
        return false;
    }

    // Create and link program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check program linking
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR: Shader program linking failed\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    // Clean up
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
  int uniformCount;
    glGetProgramiv(shaderProgram, GL_ACTIVE_UNIFORMS, &uniformCount);
    
    char uniformName[256];
    for (int i = 0; i < uniformCount; i++) {
        int length, size;
        GLenum type;
        glGetActiveUniform(shaderProgram, (GLuint)i, sizeof(uniformName), &length, &size, &type, uniformName);
        
        GLint location = glGetUniformLocation(shaderProgram, uniformName);
        std::cout << "Uniform found: " << uniformName << " (location: " << location << ")" << std::endl;
    }

    return true;
}
void Shader::useShader() {
    glUseProgram(shaderProgram);
    
    // Debug uniform locations
    GLint textureLocation = glGetUniformLocation(shaderProgram, "screenTexture");
    if (textureLocation == -1) {
        std::cerr << "Failed to find 'screenTexture' uniform" << std::endl;
    }
}
void Shader::setFloat(const std::string& name, float value) {
    GLint location = glGetUniformLocation(shaderProgram, name.c_str());
    if (location == -1) {
        std::cerr << "Warning: Uniform '" << name << "' not found in shader program" << std::endl;
        return;
    }
    glUniform1f(location, value);
}
void Shader::setMatrix4fv(const std::string& name, const float* matrix) {
    GLint location = glGetUniformLocation(shaderProgram, name.c_str());
    if (location == -1) {
        std::cerr << "Warning: Matrix uniform '" << name << "' not found" << std::endl;
        return;
    }
    glUniformMatrix4fv(location, 1, GL_FALSE, matrix);
}
void Shader::setInt(const std::string& name, int value) {
    GLint location = glGetUniformLocation(shaderProgram, name.c_str());
    if (location == -1) {
        std::cerr << "Warning: Uniform '" << name << "' not found in shader program" << std::endl;
        return;
    }
    glUniform1i(location, value);
}