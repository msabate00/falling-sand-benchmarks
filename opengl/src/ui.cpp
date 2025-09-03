#include "ui.h"
#include <glad/gl.h>
#include <cstring>
#include "material.h"
#include "engine.h"


static unsigned makeShader(unsigned type, const char* src) {
	unsigned s = glCreateShader(type); glShaderSource(s, 1, &src, nullptr); glCompileShader(s); return s;
}
static unsigned makeProgram(const char* vs, const char* fs) {
	unsigned v = makeShader(GL_VERTEX_SHADER, vs);
	unsigned f = makeShader(GL_FRAGMENT_SHADER, fs);
	unsigned p = glCreateProgram(); glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
	glDeleteShader(v); glDeleteShader(f); return p;
}

static const char* VS = R"(#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec4 aCol; // normalizado desde UNORM8
out vec4 vCol;
uniform vec2 uView; // (w,h) en px
void main(){
vCol = aCol;
vec2 p = aPos / uView * 2.0 - 1.0;
gl_Position = vec4(p.x, -p.y, 0.0, 1.0);
}
)";


static const char* FS = R"(#version 330 core
in vec4 vCol;
out vec4 o;
void main(){ o = vCol; }
)";

void UI::init() {
	prog = makeProgram(VS, FS);
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 4 * 1024 * 1024, nullptr, GL_DYNAMIC_DRAW); 


	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);


	glEnableVertexAttribArray(1); 
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)offsetof(Vertex, rgba));


	glBindVertexArray(0);
	loc_uView = glGetUniformLocation(prog, "uView");
}


void UI::shutdown() {
	if (vbo) glDeleteBuffers(1, &vbo);
	if (vao) glDeleteVertexArrays(1, &vao);
	if (prog) glDeleteProgram(prog);
	vbo = vao = prog = 0;
}


void UI::setMouse(double x, double y, bool down) {
	mdPrev = md; md = down; mx = x; my = y; mouseConsumed = false;
}


void UI::begin(int viewW, int viewH) {
	vw = viewW; vh = viewH; verts.clear();
}

void UI::draw(Engine& E, int& brushSize, Material& brushMat) {
	float pad = 8.0f, y = 8.0f, x = 8.0f, btn = 28.0f;
	auto makeBtn = [&](uint32_t base) {
		uint32_t h = MulRGBA(base, 1.15f), a = MulRGBA(base, 0.85f);
		bool clicked = button(x, y, btn, btn, base, h, a);
		x += btn + 6.0f; return clicked;
	};

	
	for (int i = 0; i < 256; ++i) {
		const MatProps& mp = matProps((u8)i);

		if (mp.name.length() >0) {
			uint32_t c = RGBAu32(mp.r, mp.g, mp.b, 230);
			if (makeBtn(c)) brushMat = (Material)i;
		}
		
	}
	
	x += 8.0f;

	if (E.paused) {
		if (makeBtn(RGBAu32(250, 200, 200, 230))) E.paused = false;
		if (makeBtn(RGBAu32(180, 220, 180, 230))) E.stepOnce = true;
	}
	else {
		if (makeBtn(RGBAu32(200, 200, 200, 230))) E.paused = true;
	}


	x += 12.0f; float bx = x, bw = 200.0f, bh = 20.0f; float v = (float)brushSize;
	slider(bx, y + 4.0f, bw, bh, 1.0f, 64.0f, v, RGBAu32(90, 90, 100, 200), RGBAu32(230, 230, 240, 240));
	brushSize = (int)(v + 0.5f);
	
}


void UI::flush() {
	if (verts.empty()) return;
	glUseProgram(prog);
	glUniform2f(loc_uView, (float)vw, (float)vh);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, verts.size() * sizeof(Vertex), verts.data());


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDrawArrays(GL_TRIANGLES, 0, (GLint)verts.size());
}


void UI::end() { flush(); }


void UI::rect(float x, float y, float w, float h, uint32_t c) {
	Vertex v[6] = {
	{x, y, c}, {x + w, y, c}, {x + w, y + h, c},
	{x, y, c}, {x + w, y + h, c}, {x, y + h, c},
	};
	verts.insert(verts.end(), v, v + 6);
}


bool UI::button(float x, float y, float w, float h,
	uint32_t c, uint32_t cH, uint32_t cA) {
	bool hover = (mx >= x && mx <= x + w && my >= y && my <= y + h);
	uint32_t cc = hover ? (md ? cA : cH) : c;
	rect(x, y, w, h, cc);


	bool clicked = hover && !md && mdPrev;
	if (hover && (md || clicked)) mouseConsumed = true;
	return clicked;
}


bool UI::slider(float x, float y, float w, float h,
	float minv, float maxv, float& v,
	uint32_t track, uint32_t knob) {
	if (v < minv) v = minv; if (v > maxv) v = maxv;
	rect(x, y + h * 0.4f, w, h * 0.2f, track);
	float t = (v - minv) / (maxv - minv);
	float kx = x + t * w;
	float kw = h;
	float kx0 = kx - kw * 0.5f;
	rect(kx0, y, kw, h, knob);


	bool hover = (mx >= x && mx <= x + w && my >= y && my <= y + h);
	if (hover && md) { v = minv + float((mx - x) / w) * (maxv - minv); mouseConsumed = true; }
	return hover && !md && mdPrev; // soltado sobre el slider
}