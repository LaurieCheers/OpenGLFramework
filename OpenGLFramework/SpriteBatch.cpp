#define GLEW_STATIC
#include <GL/glew.h>

#include "include/SpriteBatch.h"

const int VERTEX_BUFFER_STRIDE = 2 * 3 * 3; // 2 triangles x 3 vertices x XYZ
const int UV_BUFFER_STRIDE = 2 * 3 * 2; // 2 triangles x 3 vertices x XY
const int VERTEXCOL_BUFFER_STRIDE = 2 * 3 * 4; // 2 triangles x 3 vertices x RGBA

SpriteBatch::SpriteBatch(
	int maxSprites, glm::mat4& projectionMatrix, GLuint programID, GLuint texture,
	const char* positionAttribute,
	const char* mvpAttribute,
	const char* samplerAttribute,
	const char* uvAttribute,
	char* vertexColAttribute):
		programID(programID),
		maxSprites(maxSprites),
		texture(texture),
		projectionMatrix(projectionMatrix)
		//	GLuint programID, const char* positionAttribute, int maxSprites) : programID(programID), maxSprites(maxSprites)
{
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);

	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);

	glGenBuffers(1, &vertexColBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexColBuffer);

	vertices = new float[maxSprites * VERTEX_BUFFER_STRIDE];
	uvs = new float[maxSprites * UV_BUFFER_STRIDE];
	vertexCols = new float[maxSprites * VERTEXCOL_BUFFER_STRIDE];

	vertexPosition_modelspaceID = glGetAttribLocation(programID, positionAttribute);
	vertexUVID = glGetAttribLocation(programID, uvAttribute);
	vertexColAttributeID = glGetAttribLocation(programID, vertexColAttribute);

	mvpAttributeID = glGetUniformLocation(programID, mvpAttribute);
	textureAttributeID = glGetUniformLocation(programID, samplerAttribute);

	glVertexAttribPointer(
		vertexPosition_modelspaceID, // The attribute we want to configure
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);
	glVertexAttribPointer(
		vertexUVID, // The attribute we want to configure
		2,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
		);
	glVertexAttribPointer(
		vertexColAttributeID, // The attribute we want to configure
		4,                  // size
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

void SpriteBatch::DrawCenteredSizedRotated(Rectangle uvs, Vector2 pos, Vector2 size, Vector2 up, ColorRGBA col)
{
	//FIXME
//	Draw(uvs, pos, col);
	float SCALE = 0.07f;
	sprites.push_back(Sprite(uvs, pos, Vector2(size.x*SCALE, size.y*SCALE), up, col));
}

void SpriteBatch::DrawCenteredScaledRotated(Rectangle uvs, Vector2 pos, Vector2 scale, Vector2 up, ColorRGBA col)
{
	//FIXME
//	Draw(uvs, pos, col);
	sprites.push_back(Sprite(uvs, pos, scale, up, col));
}

void SpriteBatch::Draw(Rectangle rect, ColorRGBA col)
{
	sprites.push_back(Sprite(Rectangle(0,0,1,1), rect, col));
}

void SpriteBatch::Draw(Rectangle uvs, Rectangle rect, ColorRGBA col)
{
	sprites.push_back(Sprite(uvs, rect, col));
}

void SpriteBatch::Draw(Rectangle uvs, Vector2 pos, ColorRGBA col)
{
	sprites.push_back(Sprite(uvs, Rectangle(pos.x, pos.y, 100,100), col));
}

void SpriteBatch::End()
{
	int currentVertex = 0;
	int currentUV = 0;
	int currentVertexCol = 0;
	for (auto iter = sprites.begin(); iter != sprites.end(); ++iter)
	{
		for (int vtxIdx = 0; vtxIdx < sizeof(iter->vertices) / sizeof(float); ++vtxIdx)
		{
			vertices[currentVertex++] = iter->vertices[vtxIdx];
		}
		for (int uvIdx = 0; uvIdx < sizeof(iter->uvs) / sizeof(float); ++uvIdx)
		{
			uvs[currentUV++] = iter->uvs[uvIdx];
		}
		for (int vtxColIdx = 0; vtxColIdx < 6; ++vtxColIdx)
		{
			vertexCols[currentVertexCol++] = iter->color.red;
			vertexCols[currentVertexCol++] = iter->color.green;
			vertexCols[currentVertexCol++] = iter->color.blue;
			vertexCols[currentVertexCol++] = iter->color.alpha;
		}
	}

	FinalDraw();
}

void SpriteBatch::FinalDraw()
{
	glUseProgram(programID);

	glUniformMatrix4fv(mvpAttributeID, 1, GL_FALSE, &projectionMatrix[0][0]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(textureAttributeID, 0);

	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(vertexPosition_modelspaceID);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sprites.size() * sizeof(float) * VERTEX_BUFFER_STRIDE, vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(
		vertexPosition_modelspaceID, // The attribute we want to configure
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
		);

	glEnableVertexAttribArray(vertexUVID);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, sprites.size() * sizeof(float) * UV_BUFFER_STRIDE, uvs, GL_STATIC_DRAW);
	glVertexAttribPointer(
		vertexUVID, // The attribute we want to configure
		2,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
		);

	glEnableVertexAttribArray(vertexColAttributeID);
	glBindBuffer(GL_ARRAY_BUFFER, vertexColBuffer);
	glBufferData(GL_ARRAY_BUFFER, sprites.size() * sizeof(float) * VERTEXCOL_BUFFER_STRIDE, vertexCols, GL_STATIC_DRAW);
	glVertexAttribPointer(
		vertexColAttributeID, // The attribute we want to configure
		4,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
		);

	// Draw the triangle !
	glDrawArrays(GL_TRIANGLES, 0, sprites.size() * 6);

	glDisableVertexAttribArray(vertexPosition_modelspaceID);
	glDisableVertexAttribArray(vertexUVID);
}


Sprite::Sprite(Rectangle uvs, Rectangle rect, ColorRGBA col): color(col)
{
	int vtxIdx = 0;
	vertices[vtxIdx++] = rect.x;
	vertices[vtxIdx++] = rect.y;
	vertices[vtxIdx++] = 0;

	vertices[vtxIdx++] = rect.x;
	vertices[vtxIdx++] = rect.y + rect.h;
	vertices[vtxIdx++] = 0;

	vertices[vtxIdx++] = rect.x+rect.w;
	vertices[vtxIdx++] = rect.y;
	vertices[vtxIdx++] = 0;


	vertices[vtxIdx++] = rect.x;
	vertices[vtxIdx++] = rect.y + rect.h;
	vertices[vtxIdx++] = 0;

	vertices[vtxIdx++] = rect.x + rect.w;
	vertices[vtxIdx++] = rect.y + rect.h;
	vertices[vtxIdx++] = 0;

	vertices[vtxIdx++] = rect.x + rect.w;
	vertices[vtxIdx++] = rect.y;
	vertices[vtxIdx++] = 0;

	initUVs(uvs);
}

Sprite::Sprite(Rectangle uvs, Vector2 pos, Vector2 scale, Vector2 up, ColorRGBA col): color(col)
{
	float baseScale = 200.0f;
/*	float upXW = baseScale*up.x*scale.y*uvs.w;
	float upYH = baseScale*up.y*scale.x*uvs.h;
	float upYW = baseScale*up.y*scale.y*uvs.w;
	float upXH = baseScale*up.x*scale.x*uvs.h;*/
	float upX = baseScale*up.x*scale.y*uvs.h;
	float upY = baseScale*up.y*scale.y*uvs.h;

	float rightX = baseScale*up.y*scale.x*uvs.w;
	float rightY = -baseScale*up.x*scale.x*uvs.w;
	int vtxIdx = 0;
	vertices[vtxIdx++] = pos.x + (upX - rightX);
	vertices[vtxIdx++] = pos.y + (upY - rightY);
	vertices[vtxIdx++] = 0;

	vertices[vtxIdx++] = pos.x + (-upX - rightX);
	vertices[vtxIdx++] = pos.y + (-upY - rightY);
	vertices[vtxIdx++] = 0;

	vertices[vtxIdx++] = pos.x + (upX + rightX);
	vertices[vtxIdx++] = pos.y + (upY + rightY);
	vertices[vtxIdx++] = 0;


	vertices[vtxIdx++] = pos.x + (-upX - rightX);
	vertices[vtxIdx++] = pos.y + (-upY - rightY);
	vertices[vtxIdx++] = 0;

	vertices[vtxIdx++] = pos.x + (-upX + rightX);
	vertices[vtxIdx++] = pos.y + (-upY + rightY);
	vertices[vtxIdx++] = 0;

	vertices[vtxIdx++] = pos.x + (upX + rightX);
	vertices[vtxIdx++] = pos.y + (upY + rightY);
	vertices[vtxIdx++] = 0;

	initUVs(uvs);
}

void Sprite::initUVs(Rectangle uvRect)
{
	int uvIdx = 0;
	uvs[uvIdx++] = uvRect.x;
	uvs[uvIdx++] = uvRect.y;

	uvs[uvIdx++] = uvRect.x;
	uvs[uvIdx++] = uvRect.y + uvRect.h;

	uvs[uvIdx++] = uvRect.x + uvRect.w;
	uvs[uvIdx++] = uvRect.y;


	uvs[uvIdx++] = uvRect.x;
	uvs[uvIdx++] = uvRect.y + uvRect.h;

	uvs[uvIdx++] = uvRect.x + uvRect.w;
	uvs[uvIdx++] = uvRect.y + uvRect.h;

	uvs[uvIdx++] = uvRect.x + uvRect.w;
	uvs[uvIdx++] = uvRect.y;
}
