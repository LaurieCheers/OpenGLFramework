#define GLEW_STATIC
#include <GL/glew.h>

#include <GLFW/glfw3.h>
#include "MyGame.h"
#include <ctime>

GLFWwindow* CreateWindow(Game* game);
void GameLoop(GLFWwindow* window, Game* game);

/*int main(void)
{
	Game* game = new MyGame();
	CreateWindow(game);

	game->init();

	glfwTerminate();
	return 0;
}*/

GLFWwindow* CreateWindow(Game* game)
{
	/* Initialize the library */
	if (!glfwInit())
		return NULL;

	/* Create a windowed mode window and its OpenGL context */
	GLFWwindow* window = glfwCreateWindow(game->windowSize.x, game->windowSize.y, game->windowTitle.c_str(), NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return NULL;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	GLenum glewStatus = glewInit();
	if (glewStatus != GLEW_OK)
	{
		fprintf(stderr, "Failed to initialize GLEW\n");
		return NULL;
	}

	return window;
}

void GameLoop(GLFWwindow* window, Game* game)
{
	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		static std::clock_t lastTick = std::clock();

		std::clock_t now = std::clock();
		if (now - lastTick > (CLOCKS_PER_SEC / 30.0f))
		{
			game->tick();
			lastTick = now;
		}

		/* Render here */
		game->draw();

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}
}
