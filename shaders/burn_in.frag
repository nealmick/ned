#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D currentFrame;
uniform sampler2D previousFrame;
uniform float decay;

void main() {
    vec3 current = texture(currentFrame, TexCoords).rgb;
    vec3 accum = texture(previousFrame, TexCoords).rgb;
    
    // Exponential decay with adjustable power (4.0 for steep initial drop)
    vec3 result = max(current, accum * pow(decay, 4.0));
    FragColor = vec4(clamp(result, vec3(0.0), vec3(1.0)), 1.0);
}