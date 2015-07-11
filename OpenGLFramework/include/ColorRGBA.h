#pragma once

class ColorRGB
{
public:
	float r;
	float g;
	float b;

	ColorRGB()
	{
		r = 1.0f; g = 1.0f; b = 1.0f;
	}
};

struct ColorRGBA
{
public:
	float r, g, b, a;

	ColorRGBA(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}
};
