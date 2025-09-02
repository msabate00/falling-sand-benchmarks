#version 330 core
in vec2 uv;
out vec4 o;
uniform usampler2D uTex;   // R8UI índices
uniform vec2 uGrid;        // w,h del grid
uniform vec2 uView;        // w,h del viewport
layout(std140) uniform Palette { vec4 colors[256]; };

void main(){
  vec2 scale = floor(uView / uGrid);
  float s = max(1.0, min(scale.x, scale.y));
  vec2 size = uGrid * s;
  vec2 off  = (uView - size) * 0.5;

  vec2 frag = gl_FragCoord.xy - off;
  if (any(lessThan(frag, vec2(0))) || any(greaterThanEqual(frag, size))) discard;

  vec2 uv2 = frag / size;
  uint m = texture(uTex, vec2(uv2.x, 1.0 - uv2.y)).r;

  if (m==0u) discard;
  vec4 c = colors[int(m)];
  if (c.a <= 0.0) discard;
  o = c;
}
