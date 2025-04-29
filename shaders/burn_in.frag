#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D currentFrame;
uniform sampler2D previousFrame;
uniform float decay;

void main() {
    vec3 current = texture(currentFrame, TexCoords).rgb;
    vec3 accum = texture(previousFrame, TexCoords).rgb;
    
    // Use max to prevent additive brightness, allowing trails to fade
    vec3 result = max(current, accum * decay);
    FragColor = vec4(clamp(result, vec3(0.0), vec3(1.0)), 1.0);
}