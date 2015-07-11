#define GLEW_STATIC
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>

#include "WalkerGame.h"
#include <ContentLoader.h>
#include <SpriteBatch.h>

WalkerGame::WalkerGame()
{
	windowTitle = "My Awesome Game";
	windowSize = Vector2(800, 600);
}

extern GameShape* rbinit(float x, float y, float radius, float drag, float bounciness, float rolldrag, float gravity, GameColor color, float walkspeed);
extern void initGameObjects();
extern void tickPlay();

void WalkerGame::init()
{
	int numParticles = 100;

//	spriteBatch = new SpriteBatch();
	spriteBatch = new SpriteBatch(
		numParticles,
		glm::ortho(0.0f, windowSize.x, windowSize.y, 0.0f),
		content->LoadShaders("VColVertexShader.vertexshader", "VColTextureFragmentShader.fragmentshader"),
		content->LoadBMP("Walker.bmp"),
		"vertexPosition_modelspace", "MVP", "myTextureSampler", "vertexUV",
		"vertexColor"
	);
	
	Vector2 textureSize(512, 512);
	spriteBatch->textureSize = textureSize;

	whiteTile.initXYXY(textureSize, 5, 5, 25, 25);
	starTile.initXYXY(textureSize, 40, 25, 85, 63);

	walk1Tile.initXYXY(textureSize, 120, 18, 152, 66);
	walk2Tile.initXYXY(textureSize, 232, 18, 258, 66);
	fallTile.initXYXY(textureSize, 170, 2, 215, 82);
	climb1Tile.initXYXY(textureSize, 316, 18, 337, 66);
	climb2Tile.initXYXY(textureSize, 351, 18, 371, 66);

//	rbTest = rbinit(100, 150, 15, 0, 0.05f, 1, 0.05f, COL_Blue, 0.95f);
	initGameObjects();

	float particleSize = 0.05f;
	for (int Idx = 0; Idx < numParticles; ++Idx)
	{
		particles.push_back(Rectangle(rand() % 2000 * 0.001f - 1.0f, rand() % 2000 * 0.001f - 1.0f, particleSize, particleSize));
	}
}

void WalkerGame::tick()
{
	tickPlay();
}

void WalkerGame::draw()
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	spriteBatch->Begin();
	drawView(spriteBatch);
	spriteBatch->End();

}