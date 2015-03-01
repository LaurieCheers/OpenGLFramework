#define GLEW_STATIC
#include <GL/glew.h>

#include "SpriteBatch.h"

SpriteBatch::SpriteBatch(GLuint programID, const char* positionAttribute, int maxSprites) : programID(programID), maxSprites(maxSprites)
{
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);

	vertices = new float[maxSprites * 18]; // 18 = 2 triangles x 3 vertices x 3d

	vertexPosition_modelspaceID = glGetAttribLocation(programID, positionAttribute);
	glVertexAttribPointer(
		vertexPosition_modelspaceID, // The attribute we want to configure
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);
}

void SpriteBatch::Begin()
{
	sprites.clear();
}

void SpriteBatch::Draw(Rectangle rect)
{
	sprites.push_back(Sprite(rect));
}

void SpriteBatch::End()
{
	int currentVertex = 0;
	for (auto iter = sprites.begin(); iter != sprites.end(); ++iter)
	{
		Rectangle rect = iter->rect;

		vertices[currentVertex++] = rect.x;
		vertices[currentVertex++] = rect.y;
		vertices[currentVertex++] = 0;

		vertices[currentVertex++] = rect.x;
		vertices[currentVertex++] = rect.y + rect.h;
		vertices[currentVertex++] = 0;

		vertices[currentVertex++] = rect.x + rect.w;
		vertices[currentVertex++] = rect.y;
		vertices[currentVertex++] = 0;


		vertices[currentVertex++] = rect.x + rect.w;
		vertices[currentVertex++] = rect.y;
		vertices[currentVertex++] = 0;

		vertices[currentVertex++] = rect.x + rect.w;
		vertices[currentVertex++] = rect.y + rect.h;
		vertices[currentVertex++] = 0;

		vertices[currentVertex++] = rect.x;
		vertices[currentVertex++] = rect.y + rect.h;
		vertices[currentVertex++] = 0;
	}

	FinalDraw();
}

void SpriteBatch::FinalDraw()
{
	// Use our shader
	glUseProgram(programID);

	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(vertexPosition_modelspaceID);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, maxSprites * 18 * sizeof(float), vertices, GL_DYNAMIC_DRAW);

	glDrawArrays(GL_TRIANGLES, 0, sprites.size() * 6);

	glDisableVertexAttribArray(vertexPosition_modelspaceID);
}