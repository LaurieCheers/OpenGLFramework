#define GLEW_STATIC
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include "MyGame.h"

GLFWwindow* CreateWindow(Game* game);
void GameLoop(GLFWwindow* window, Game* game);

int main()
{
	MyGame* game = new MyGame();

	GLFWwindow* window = CreateWindow(game);
	game->init();
	GameLoop(window, game);
}