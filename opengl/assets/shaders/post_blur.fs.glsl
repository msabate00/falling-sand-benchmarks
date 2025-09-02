#version 330 core
in vec2 uv; out vec4 o;
uniform sampler2D uTex;
uniform vec2 uTexel;      // 1/size
uniform int  uHorizontal; // 1: X, 0: Y

void main(){
    vec2 dir = (uHorizontal==1) ? vec2(1,0) : vec2(0,1);
    float w[5] = float[5](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
    vec3 c = texture(uTex, uv).rgb * w[0];
    for(int i=1;i<5;i++){
        c += texture(uTex, uv + dir*uTexel*float(i)).rgb*w[i];
        c += texture(uTex, uv - dir*uTexel*float(i)).rgb*w[i];
    }
    o = vec4(c,1.0);
}
