#pragma once

#include <cmath>

class Vector2
{
public:
	float x, y;

	Vector2(float x, float y) : x(x), y(y)
	{

	}

	float LengthSquared()
	{
		return x*x + y*y;
	}

	float Length()
	{
		return sqrt(x*x + y*y);
	}
};