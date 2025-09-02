#version 330 core
in vec2 uv;
out vec4 o;

uniform usampler2D uTex;   // índices R8UI
uniform vec2 uGrid;        // (w,h)
uniform vec2 uView;        // viewport px
layout(std140) uniform Palette { vec4 colors[256]; };

// hash determinista por celda
float hash2(ivec2 p){
    uint x = uint(p.x)*374761393u ^ uint(p.y)*668265263u;
    x = (x ^ (x>>13)) * 1274126177u;
    x ^= x >> 16u;
    return float(x & 1023u) / 1023.0; // [0,1]
}

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

  // -------- variacion de color por celda --------
  ivec2 cellId = ivec2(clamp(floor(uv2 * uGrid), vec2(0), uGrid - 1.0));
  float n = hash2(cellId)*2.0 - 1.0;  // [-1,1]
  float k = 0.15;                     // intensidad (6%)
  c.rgb = clamp(c.rgb * (1.0 + k*n), 0.0, 1.0);
  // ----------------------------------------------

  vec2 cell = fract(uv2 * uGrid); 
  vec2 p = cell - vec2(0.5);        
  float r = length(p);

   // --------- Parametros de los puntos ----------
  float radius  = 0.35;
  float feather = 0.30;
  // ----------------------------------------------

  float alpha = 1.0 - smoothstep(radius, radius + feather, r);
  o = vec4(c.rgb, c.a * alpha);
  if (o.a <= 0.001) discard;
}
