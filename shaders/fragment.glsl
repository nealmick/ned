#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D screenTexture;
uniform float time;
uniform vec2 resolution;

// Improved random function that avoids patterns
float random(vec2 co) {
    float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    float dt = dot(co.xy, vec2(a,b));
    float sn = mod(dt, 3.14);
    return fract(sin(sn) * c);
}

// Additional random function for variety
float random2(vec2 co) {
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453123);
}

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

vec2 addJitter(vec2 uv, float time) {
    float jitterSpeed = 2.0;
    float jitterAmount = 0.0009;
    
    float jitterThreshold = 0.97;
    float rand = random(vec2(mod(time * 0.1, 100.0)));
    if(rand > jitterThreshold) {
        vec2 jitter = vec2(
            random(uv + mod(time * jitterSpeed, 10.0)) - 0.5,
            random(uv + mod(time * jitterSpeed + 1.0, 10.0)) - 0.5
        ) * jitterAmount;
        return uv + jitter;
    }
    return uv;
}

float generateStatic(vec2 uv, float time) {
    // Use modulo to keep time values from growing too large
    float t1 = mod(time * 60.0, 10.0);
    float t2 = mod(time * 55.0, 10.0);
    float t3 = mod(time * 0.1, 5.0);
    
    // Create multiple layers of noise at different frequencies
    float noise1 = random(uv * 2.5 + vec2(t1));
    float noise2 = random2(uv * 3.7 + vec2(t2));
    float noise3 = random((uv + vec2(t3)) * 1.5);
    
    // Combine the noise layers with different weights
    float staticNoise = (noise1 * 0.5 + noise2 * 0.3 + noise3 * 0.2);
    
    // Add time-based modulation to control the intensity
    float intensityMod = mix(0.8, 1.0, sin(time * 0.5) * 0.5 + 0.5);
    return staticNoise * intensityMod;
}

void main() {
    vec2 uv = TexCoords;
    uv = addJitter(uv, time);
    
    float shiftAmount = 0.001;
    float shiftSpeed = 0.5;
    float shift = sin(time * shiftSpeed) * shiftAmount;
    
    vec3 color;
    color.r = texture(screenTexture, uv + vec2(shift, 0.0)).r;
    color.g = texture(screenTexture, uv).g;
    color.b = texture(screenTexture, uv - vec2(shift, 0.0)).b;
    
    vec3 bloom = sampleBloom(uv, 2.0);
    float bloomStrength = 0.3;
    color += bloom * bloomStrength;
    
    vec2 vigUV = uv;
    vigUV *= 1.0 - vigUV.yx;
    float vignette = vigUV.x * vigUV.y * 15.0;
    vignette = pow(vignette, 0.15);
    
    float scanSpeed = 0.2;
    float pauseDuration = 3.0;
    float cycleTime = mod(time, 1.0/scanSpeed + pauseDuration);
    float scanline = 1.0;
    
    if(cycleTime < 1.0/scanSpeed) {
        float scanPos = cycleTime * scanSpeed;
        float scanDist = (uv.y - (1.0 - fract(scanPos))) * resolution.y;
        
        float fadeInLength = 7.0;
        float fadeOutLength = 130.0;
        
        if (scanDist >= 0.0 && scanDist < fadeOutLength) {
            float fadeOut = 1.0 - (scanDist / fadeOutLength);
            fadeOut = pow(smoothstep(0.2, 1.0, fadeOut), 1.2);
            float fadeIn = min(scanDist / fadeInLength, 1.0);
            fadeIn = smoothstep(0.0, 1.0, fadeIn);
            float combinedFade = fadeIn * fadeOut;
            scanline = mix(1.0, 1.7, combinedFade * 0.2);
        }
    }
    
    color *= vignette * scanline;
    
    vec2 staticUV = gl_FragCoord.xy / resolution;
    float staticNoise = generateStatic(staticUV, time);
    color += (staticNoise - 0.5) * 0.09; // Slightly reduced intensity
    
    float pulseSpeed = 0.5;
    float pulseStrength = 0.05;
    float pulse = sin(time * pulseSpeed) * pulseStrength + 1.0;
    color *= pulse;
    
    FragColor = vec4(color, 1.0);
}