#version 330 core

in float vHeight;
out vec4 FragColor;

uniform float uMinHeight;
uniform float uMaxHeight;

const float T1 = 0.25f;
const float T2 = 0.5f;
const float T3 = 0.75f;

const vec3 darkGreen = vec3(0.0, 0.4, 0.0);
const vec3 brightGreen = vec3(0.0, 1.0, 0.0);
const vec3 brightRed = vec3(1.0, 0.0, 0.0);
const vec3 whiteColor = vec3(1.0, 1.0, 1.0);

void main() {
    
    if(vHeight < -500 || vHeight > 9000) {
        FragColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }

    float normalized = clamp((vHeight - uMinHeight) / (uMaxHeight - uMinHeight), 0.0f, 1.0f);

    vec3 finalColor;

    if(normalized < T1) {
        float t = normalized / T1;
        finalColor = mix(darkGreen, brightGreen, t);
    }
    else if(normalized < T2) {
        float t = (normalized - T1) / (T2 - T1);
        finalColor = mix(brightGreen, brightRed, t);
    }
    else if(normalized < T3) {
        float t = (normalized - T2) / (T3 - T2);
        finalColor = mix(brightRed, whiteColor, t);
    }
    else {
        finalColor = whiteColor;
    }

    FragColor = vec4(finalColor, 1.0f);
}
