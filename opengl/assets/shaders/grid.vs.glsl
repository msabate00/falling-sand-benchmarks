#version 330 core
const vec2 V[3]=vec2[3](vec2(-1,-1),vec2(3,-1),vec2(-1,3));
out vec2 uv;
void main(){
  gl_Position = vec4(V[gl_VertexID],0,1);
  uv = gl_Position.xy*0.5 + 0.5;
}
