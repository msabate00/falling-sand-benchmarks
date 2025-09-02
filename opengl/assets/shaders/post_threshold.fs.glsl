#version 330 core
in vec2 uv; out vec4 o;
uniform sampler2D uScene;
uniform float uThreshold = 1.0; // brillo para bloom
void main(){
    vec3 c = texture(uScene, uv).rgb;
    float luma = dot(c, vec3(0.2126,0.7152,0.0722));
    o = (luma > uThreshold) ? vec4(c,1.0) : vec4(0.0);
}
