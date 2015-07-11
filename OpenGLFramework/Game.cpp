#define GLEW_STATIC
#include <GL/glew.h>

#include "Game.h"
#include "ContentLoader.h"

Game::Game() : windowSize(640, 480)
{
	content = new ContentLoader();
	windowTitle = "Game";
}

void Game::init()
{

}

void Game::tick()
{

}

void Game::draw()
{
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}