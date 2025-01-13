#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float time;

// Simple noise function
float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main() {
    vec2 uv = TexCoords;
    
   

    // Slight color separation
    float r = texture(screenTexture, uv + vec2(0.005 * sin(time), 0.0)).r;
    float g = texture(screenTexture, uv).g;
    float b = texture(screenTexture, uv - vec2(0.005 * sin(time), 0.0)).b;

    // Combine color channels
    vec3 color = vec3(r, g, b);

    // Scanline effect
    float scanline = sin(uv.y * 240.0 + time * 10.0) * 0.05;
    color += scanline;

    // Noise/static effect
    float noise = (random(uv + time) - 0.5) * 0.05;
    color += noise;

    // Vignette effect
    float vignette = 1.0 - dot(uv - 0.5, uv - 0.5) * 2.0;
    color *= vignette;

    // Slight color desaturation
    float grayscale = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(grayscale), color, 0.8);

    FragColor = vec4(color, 1.0);
}
