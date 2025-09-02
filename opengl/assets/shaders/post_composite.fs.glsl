#version 330 core
in vec2 uv; out vec4 o;
uniform sampler2D uScene;
uniform sampler2D uBloom;
uniform float uExposure = 1.0;
uniform float uBloomStrength = 0.7;

void main(){
    vec3 c = texture(uScene, uv).rgb + uBloomStrength*texture(uBloom, uv).rgb;
    // tonemap (ACES-ish simple): 1-exp(-x*exposure), luego gamma 2.2
    c = vec3(1.0) - exp(-c * uExposure);
    c = pow(c, vec3(1.0/2.2));
    o = vec4(c,1.0);
}
