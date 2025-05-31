#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D screenTexture;
uniform float time;
uniform vec2 resolution;

uniform float u_scanline_intensity;
uniform float u_vignet_intensity;
uniform float u_bloom_intensity;
uniform float u_static_intensity;
uniform float u_colorshift_intensity;
uniform float u_jitter_intensity;
uniform float u_curvature_intensity;
uniform float u_pixelation_intensity;
uniform float u_pixel_width;
uniform float u_effects_enabled;


float random(vec2 co) {
    float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    float dt = dot(co.xy, vec2(a,b));
    float sn = mod(dt, 3.14);
    return fract(sin(sn) * c);
}

float random2(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453123);
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
    float baseJitterAmount = 0.0009;
    float jitterAmount = baseJitterAmount * u_jitter_intensity;
    
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
    float t1 = mod(time * 60.0, 10.0);
    float t2 = mod(time * 55.0, 10.0);
    float t3 = mod(time * 0.1, 5.0);
    
    float noise1 = random(uv * 2.5 + vec2(t1));
    float noise2 = random2(uv * 3.7 + vec2(t2));
    float noise3 = random((uv + vec2(t3)) * 1.5);
    
    float staticNoise = (noise1 * 0.5 + noise2 * 0.3 + noise3 * 0.2);
    float intensityMod = mix(0.8, 1.0, sin(time * 0.5) * 0.5 + 0.5);
    return staticNoise * intensityMod;
}



vec3 getBloom(vec2 uv) {
    return sampleBloom(uv, 2.0) * u_bloom_intensity;
}

float applyVignette(vec2 uv) {
    vec2 vigUV = uv;
    vigUV *= 1.0 - vigUV.yx;
    float vignette = vigUV.x * vigUV.y * 15.0;
    return pow(vignette, u_vignet_intensity);
}

float calculateScanline(vec2 uv, float time) {
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
            scanline = mix(1.0, 1.7, combinedFade * u_scanline_intensity);
        }
    }
    return scanline;
}

vec3 applyStaticNoise(vec3 color, float time) {
    vec2 staticUV = gl_FragCoord.xy / resolution;
    float staticNoise = generateStatic(staticUV, time);
    return color + (staticNoise - 0.5) * u_static_intensity;
}

vec3 applyPulse(vec3 color, float time) {
    float pulseSpeed = 0.5;
    float pulseStrength = 0.05;
    float pulse = sin(time * pulseSpeed) * pulseStrength + 1.0;
    return color * pulse;
}
vec2 applyCurvature(vec2 uv, float intensity) {
    // Convert to polar coordinates
    vec2 center = uv - 0.5;
    float radius = length(center);
    float angle = atan(center.y, center.x);
    
    // Create proper outward bulge distortion
    float distortion = intensity * 0.25 * pow(radius, 2.0);
    radius *= 1.0 + distortion;
    
    // Convert back to Cartesian coordinates
    vec2 distorted = 0.5 + radius * vec2(cos(angle), sin(angle));
    
    // Maintain valid texture coordinates
    return clamp(distorted, 0.001, 0.999);
}


vec3 applyColorShift(vec2 uv, float time) {
    float baseShiftAmount = 0.001;
    float shiftAmount = baseShiftAmount * u_colorshift_intensity;
    float shiftSpeed = 0.5;
    float shift = sin(time * shiftSpeed) * shiftAmount;

    vec3 color;
    color.r = texture(screenTexture, uv + vec2(shift * 1.0, 0.0)).r;
    color.g = texture(screenTexture, uv + vec2(shift * 0.3, 0.0)).g;
    color.b = texture(screenTexture, uv - vec2(shift * 1.0, 0.0)).b;
    return color;
}



vec3 pixelate(vec2 uv) {
    // Fixed grid size of 250 cells
    vec2 cellSizeUV = vec2(1.0) / vec2(u_pixel_width);
    vec2 cell = floor(uv * u_pixel_width);
    vec2 centerUV = (cell + 0.5) * cellSizeUV;
    return texture(screenTexture, centerUV).rgb;
}

vec3 applyGrid(vec3 color) {
    vec2 cellSize = resolution / 250.0;
    vec2 gridPos = mod(gl_FragCoord.xy, cellSize);
    float lineThickness = 1.0;
    
    // Draw grid lines using pixelation intensity for opacity
    if (gridPos.x < lineThickness || gridPos.y < lineThickness) {
        color = mix(color, vec3(0.0), u_pixelation_intensity);
    }
    
    return color;
}

void main() {
    if (u_effects_enabled > 0.5) {
        vec2 uv = TexCoords;

        uv = applyCurvature(uv, u_curvature_intensity);
        uv = addJitter(uv, time);


        vec3 color = pixelate(uv);
        color = applyColorShift(uv, time);

        color += getBloom(uv);
        color *= applyVignette(uv) * calculateScanline(uv, time);
        color = applyStaticNoise(color, time);
        color = applyPulse(color, time);
        color = applyGrid(color);

        FragColor = vec4(color, 1.0);
    } else {
        FragColor = texture(screenTexture, TexCoords);
    }
}