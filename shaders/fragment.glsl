#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D screenTexture;
uniform float time;
uniform vec2 resolution;

float random(vec2 uv) {
    return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453123);
}

// Simple gaussian blur approximation
vec3 sampleBloom(vec2 uv, float offset) {
    vec3 bloom = vec3(0.0);
    float total = 0.0;
    
    for(float x = -2.0; x <= 2.0; x += 1.0) {
        for(float y = -2.0; y <= 2.0; y += 1.0) {
            vec2 sampleUV = uv + vec2(x, y) * offset / resolution;
            float weight = 1.0 - length(vec2(x, y)) * 0.1;
            if(weight > 0.0) {
                bloom += texture(screenTexture, sampleUV).rgb * weight;
                total += weight;
            }
        }
    }
    
    return bloom / total;
}

// Add random jitter function
vec2 addJitter(vec2 uv, float time) {
    float jitterSpeed = 2.0;
    float jitterAmount = 0.0009; // Very subtle jitter
    
    // Only jitter occasionally
    float jitterThreshold = 0.97;
    float rand = random(vec2(time * 0.1));
    if(rand > jitterThreshold) {
        vec2 jitter = vec2(
            random(uv + time * jitterSpeed) - 0.5,
            random(uv + time * jitterSpeed + 1.0) - 0.5
        ) * jitterAmount;
        return uv + jitter;
    }
    return uv;
}

void main() {
    vec2 uv = TexCoords;
    
    // Add occasional jitter
    uv = addJitter(uv, time);
    
    // Subtle RGB shift
    float shiftAmount = 0.001; // Very subtle shift
    float shiftSpeed = 0.5;
    float shift = sin(time * shiftSpeed) * shiftAmount;
    
    // Sample colors with RGB shift
    vec3 color;
    color.r = texture(screenTexture, uv + vec2(shift, 0.0)).r;
    color.g = texture(screenTexture, uv).g;
    color.b = texture(screenTexture, uv - vec2(shift, 0.0)).b;
    
    // Add bloom effect
    vec3 bloom = sampleBloom(uv, 2.0);
    float bloomStrength = 0.3;
    color += bloom * bloomStrength;
    
    // Vignette calculation
    vec2 vigUV = uv;
    vigUV *= 1.0 - vigUV.yx;
    float vignette = vigUV.x * vigUV.y * 15.0;
    vignette = pow(vignette, 0.15);
    
    // Modified scanline timing with pause
    float scanSpeed = 0.2;
    float pauseDuration = 3.0; // Duration of pause between scans
    
    // Create a cycle that includes the scan and pause
    float cycleTime = mod(time, 1.0/scanSpeed + pauseDuration);
    float scanline = 1.0;
    
    // Only show scanline during the scan part of the cycle
    if(cycleTime < 1.0/scanSpeed) {
        float scanPos = cycleTime * scanSpeed;
        float scanDist = (uv.y - (1.0 - fract(scanPos))) * resolution.y;
        
        float fadeInLength = 7.0;  // Quick fade in
        float fadeOutLength = 130.0; // Long fade out
        
        if (scanDist >= 0.0 && scanDist < fadeOutLength) {
            float fadeOut = 1.0 - (scanDist / fadeOutLength);
            fadeOut = pow(smoothstep(0.2, 1.0, fadeOut), 1.2);
            
            // Add the fade-in effect
            float fadeIn = min(scanDist / fadeInLength, 1.0);
            fadeIn = smoothstep(0.0, 1.0, fadeIn);
            
            // Combine both fades
            float combinedFade = fadeIn * fadeOut;
            
            // Adjust the intensity range
            scanline = mix(1.0, 1.7, combinedFade * 0.2);
        }
    }
    // Apply all effects
    color *= vignette * scanline;
    color += 0.05 * (random(uv * time) - 0.5);
    
    // Add subtle pulsing to the bloom over time
    float pulseSpeed = 0.5;
    float pulseStrength = 0.05;
    float pulse = sin(time * pulseSpeed) * pulseStrength + 1.0;
    color *= pulse;
    
    FragColor = vec4(color, 1.0);
}