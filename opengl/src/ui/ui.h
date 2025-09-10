#pragma once
#include <vector>
#include <cstdint>


struct Engine;

enum class Material : std::uint8_t;


class UI {
public:
	void init();
	void shutdown();


	void begin(int viewW, int viewH); 
	void end();
	void draw(Engine& E, int& brushSize, Material& brushMat);


	void setMouse(double x, double y, bool down);
	bool consumedMouse() const { return mouseConsumed; }


	
	bool button(float x, float y, float w, float h,
		uint32_t rgba, uint32_t rgbaHover, uint32_t rgbaActive);
	bool slider(float x, float y, float w, float h,
		float minv, float maxv, float& v, uint32_t track, uint32_t knob);


	
	void rect(float x, float y, float w, float h, uint32_t rgba);


private:
	struct Vertex { float x, y; uint32_t rgba; };


	
	unsigned int prog = 0, vao = 0, vbo = 0;
	int loc_uView = -1;


	
	std::vector<Vertex> verts;
	int vw = 0, vh = 0;


	
	double mx = 0.0, my = 0.0; bool md = false, mdPrev = false;
	bool mouseConsumed = false;


	
	void flush();
};


static inline uint32_t RGBAu32(unsigned r, unsigned g, unsigned b, unsigned a = 255) {
	return (r & 255u) | ((g & 255u) << 8) | ((b & 255u) << 16) | ((a & 255u) << 24);
}


static inline uint32_t MulRGBA(uint32_t c, float m) {
	unsigned r = (unsigned)((c) & 255u);
	unsigned g = (unsigned)((c >> 8) & 255u);
	unsigned b = (unsigned)((c >> 16) & 255u);
	unsigned a = (unsigned)((c >> 24) & 255u);
	auto sat = [](int v) { return (unsigned)(v < 0 ? 0 : v > 255 ? 255 : v); };
	r = sat(int(r * m)); g = sat(int(g * m)); b = sat(int(b * m));
	return (r) | (g << 8) | (b << 16) | (a << 24);
}