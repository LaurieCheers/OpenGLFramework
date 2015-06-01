#pragma once

#include <Game.h>
#include <vector>
#include <Rectangle.h>

class SpriteBatch;

class MyGame : public Game
{
	SpriteBatch* spriteBatch;
	std::vector<Rectangle> particles;

public:
	MyGame();
	virtual void init();
	virtual void draw();
};