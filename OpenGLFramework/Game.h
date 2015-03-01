#pragma once

#include "Vector2.h"
#include <string>

class ContentLoader;

class Game
{
protected:
	ContentLoader* content;

public:
	std::string windowTitle;
	Vector2 windowSize;

	Game();
	virtual void init();
	virtual void draw();
};