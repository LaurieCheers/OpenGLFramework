#pragma once

#include <cmath>

class Vector2
{
public:
	float x, y;

	Vector2() : x(0), y(0)
	{
	}

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

	void Normalize()
	{
		float len = Length();
		if (len == 0)
		{
			x = 1;
		}
		else
		{
			x /= len;
			y /= len;
		}
	}
};