#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D previousFrameTexture; // New uniform for previous frame
uniform float time;
uniform vec2 resolution;

// Burn-in parameters
uniform float burnPersistence = 0.95; // How long colors linger
uniform float burnFadeRate = 0.02;    // How quickly burn fades

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main() {
    vec2 uv = TexCoords;
    
    // Current frame color
    vec3 currentColor = texture(screenTexture, uv).rgb;
    
    // Previous frame color (burned-in image)
    vec3 previousColor = texture(previousFrameTexture, uv).rgb;
    
    // Blend current and previous frame with burn-in effect
    vec3 burnColor = mix(previousColor * burnPersistence, currentColor, burnFadeRate);
    
    // Color separation effect
    float r = texture(screenTexture, uv + vec2(0.003 * sin(time), 0.0)).r;
    float g = burnColor.g;
    float b = texture(screenTexture, uv - vec2(0.003 * sin(time), 0.0)).b;
    vec3 color = vec3(r, g, b);
    
    // Scanline effect
    float scanPos = mod(uv.y * 1.0 + time * 0.5, 2.0);
    float scanline = smoothstep(0.0, 0.1, scanPos) * smoothstep(1.0, 0.9, scanPos) * 0.03;
    color = color - scanline;
    
    // Static noise
    float noise = (random(uv + time) - 0.5) * 0.06;
    color += noise;
    
    // Vignette effect
    float vignette = 1.0 - dot(uv - 0.5, uv - 0.5) * 1.7;
    color *= vignette;
    
    // Color adjustment
    float grayscale = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(grayscale), color, 0.85);
    
    FragColor = vec4(color, 1.0);
}