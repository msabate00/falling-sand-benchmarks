#version 330 core
in vec2 uv;
out vec4 o;

uniform usampler2D uTex;   // índices R8UI
uniform vec2 uGrid;        // (w,h)
uniform vec2 uView;        // viewport px
layout(std140) uniform Palette { vec4 colors[256]; };

void main(){
  vec2 scale = floor(uView / uGrid);
  float s = max(1.0, min(scale.x, scale.y));
  vec2 size = uGrid * s;
  vec2 off  = (uView - size) * 0.5;

  vec2 frag = gl_FragCoord.xy - off;
  if (any(lessThan(frag, vec2(0))) || any(greaterThanEqual(frag, size)))
    discard;

  vec2 uv2 = frag / size;
  uint m = texture(uTex, vec2(uv2.x, 1.0 - uv2.y)).r;
  if (m==0u) discard;

  vec4 c = colors[int(m)];
  if (c.a <= 0.0) discard;

  vec2 cell = fract(uv2 * uGrid);       // 0..1 en la celda
  vec2 p = cell - vec2(0.5);            // centro (0,0)
  float r = length(p);

  // Parámetros del “disco”
  float radius  = 0.35;                 // 0..0.5
  float feather = 0.30;                 // ancho del borde suave

  float alpha = 1.0 - smoothstep(radius, radius + feather, r);
  o = vec4(c.rgb, c.a * alpha);
  if (o.a <= 0.001) discard;
}
