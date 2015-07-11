#define GLEW_STATIC
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include "WalkerGame.h"

GLFWwindow* CreateWindow(Game* game);
void GameLoop(GLFWwindow* window, Game* game);

extern void testFn();

int main()
{
	WalkerGame* game = new WalkerGame();

	GLFWwindow* window = CreateWindow(game);
	game->init();
	GameLoop(window, game);
}