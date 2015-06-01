#define GLEW_STATIC
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>

#include "MyGame.h"
#include <ContentLoader.h>
#include <SpriteBatch.h>

MyGame::MyGame()
{
	windowTitle = "My Awesome Game";
	windowSize = Vector2(1024, 768);
}

void MyGame::init()
{
	int numParticles = 100;

	spriteBatch = new SpriteBatch(
		numParticles,
		glm::ortho(0.0f, windowSize.x, 0.0f, windowSize.y),
		content->LoadShaders("TransformVertexShader.vertexshader", "TextureFragmentShader.fragmentshader"),
		content->LoadBMP("ceo.bmp"),
		"vertexPosition_modelspace", "MVP", "myTextureSampler", "vertexUV", NULL
	);

	float particleSize = 0.05f;
	for (int Idx = 0; Idx < numParticles; ++Idx)
	{
		particles.push_back(Rectangle(rand() % 2000 * 0.001f - 1.0f, rand() % 2000 * 0.001f - 1.0f, particleSize, particleSize));
	}
}

void MyGame::draw()
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	spriteBatch->Begin();
/*	for (auto iter = particles.begin(); iter != particles.end(); ++iter)
	{
		iter->x += 0.0001f;
		iter->y += 0.0001f;

		if (iter->x > 1.0f)
			iter->x -= 2.1f;

		if (iter->y > 1.0f)
			iter->y -= 2.1f;
		
		spriteBatch->Draw(*iter);
	}*/
	spriteBatch->Draw(Rectangle(100,100, 100,100));
	spriteBatch->End();
}