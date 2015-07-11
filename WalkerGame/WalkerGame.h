#pragma once

#pragma once
#include <Game.h>
#include <vector>
#include <Rectangle.h>
#include "SpriteBatch.h"
#include "walker_core.h"

class SpriteBatch;

class WalkerGame : public Game
{
	SpriteBatch* spriteBatch;
	std::vector<Rectangle> particles;

	SpriteTile walk1Tile;
	SpriteTile walk2Tile;
	SpriteTile fallTile;
	SpriteTile climb1Tile;
	SpriteTile climb2Tile;
	SpriteTile whiteTile;
	SpriteTile starTile;

	//GameShape* rbTest;

	void drawBoundingBox(BoundingBox* box, SpriteBatch* spriteBatch);
	void drawLineShape(LineShape* line, SpriteBatch* spriteBatch);
	void drawMovable(MovableSegment* mov, SpriteBatch* spriteBatch);
	void drawStar(RigidBody* rb, SpriteBatch* spriteBatch);
	void drawMan(RigidBody* rb, SpriteBatch* spriteBatch);
	void drawObject(GameShape* shape, SpriteBatch* spriteBatch);
	void drawView(SpriteBatch* spriteBatch);

public:
	WalkerGame();
	virtual void init();
	virtual void tick();
	virtual void draw();


};