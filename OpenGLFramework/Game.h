#pragma once

#include "include/Vector2.h"
#include <string>
#include <GLFW/glfw3.h>

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
	virtual void tick();
	virtual void draw();
};