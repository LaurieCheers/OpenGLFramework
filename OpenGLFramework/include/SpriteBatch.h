#pragma once
#include "Rectangle.h"
#include "Vector2.h"
#include <vector>
#include <glm/glm.hpp>

class ColorRGB
{
public:
	float red;
	float green;
	float blue;

	ColorRGB()
	{
		red = 1.0f; green = 1.0f; blue = 1.0f;
	}
};

class ColorRGBA
{
public:
	float red;
	float green;
	float blue;
	float alpha;

	ColorRGBA()
	{
		red = 1.0f; green = 1.0f; blue = 1.0f; alpha = 1.0f;
	}
};

class Sprite
{
public:
	// 2 triangles, 3 vertices each, 3d
	float vertices[2 * 3 * 3];

	// 2 triangles, 3 vertices each, 2d 
	float uvs[2 * 3 * 2];

	ColorRGBA color;

	explicit Sprite(Rectangle uvs, Rectangle rect, ColorRGBA col);
	explicit Sprite(Rectangle uvs, Vector2 pos, Vector2 scale, Vector2 up, ColorRGBA col);

private:
	void initUVs(Rectangle uvRect);
};

struct SpriteTile
{
	Rectangle rect;

	SpriteTile() : rect(0, 0, 1, 1)
	{}

	void initXYWH(Vector2 textureSize, float x, float y, float w, float h)
	{
		rect = Rectangle(x / textureSize.x, 1 - (y / textureSize.y), x / textureSize.x, -h / textureSize.y);
	}

	void initXYXY(Vector2 textureSize, float x, float y
		, float x2, float y2)
	{
		rect = Rectangle(x / textureSize.x, 1 - (y / textureSize.y), (x2 - x) / textureSize.x, (y - y2) / textureSize.y);
	}
};

class SpriteBatch
{
public:
	std::vector<Sprite> sprites;
	GLuint programID;

	GLfloat* vertices;
	GLuint vertexbuffer;
	GLuint vertexPosition_modelspaceID;

	GLuint texture;
	GLuint textureAttributeID;

	glm::mat4 projectionMatrix;
	GLuint mvpAttributeID;
	GLuint vertexColAttributeID;

	GLfloat* uvs;
	GLuint uvbuffer;
	GLfloat* vertexCols;
	GLuint vertexColBuffer;
	GLuint vertexUVID;
//	GLuint vertexColID;
//	GLuint mvpID;

	Vector2 textureSize;

	int maxSprites;

public:
	explicit SpriteBatch(int maxSprites, glm::mat4& projectionMatrix, GLuint programID, GLuint texture, const char* positionAttribute, const char* mvpAttribute, const char* samplerAttribute, const char* uvAttribute, char* vertexColAttribute);
	explicit SpriteBatch(int maxSprites, glm::mat4& projectionMatrix, GLuint programID, const char* positionAttribute, const char* mvpAttribute) : SpriteBatch(maxSprites, projectionMatrix, programID, 0, positionAttribute, mvpAttribute, NULL, NULL, NULL){};
	~SpriteBatch();
	void Begin();
	void Draw(Rectangle rect, ColorRGBA col = ColorRGBA());
	void Draw(Rectangle uvs, Rectangle rect, ColorRGBA col = ColorRGBA());
	void Draw(Rectangle uvs, Vector2 pos, ColorRGBA col = ColorRGBA());
	void Draw(SpriteTile uvs, Rectangle rect, ColorRGBA col = ColorRGBA()) { Draw(uvs.rect, rect, col); }
	void Draw(SpriteTile uvs, Vector2 pos, ColorRGBA col = ColorRGBA()) { Draw(uvs.rect, pos, col); }
	void DrawCentered(Rectangle uvs, Vector2 pos, ColorRGBA col = ColorRGBA()) { DrawCenteredScaledRotated(uvs, pos, Vector2(1, 1), Vector2(0, 1), col); }
	void DrawCentered(SpriteTile uvs, Vector2 pos, ColorRGBA col = ColorRGBA()) { DrawCenteredScaledRotated(uvs.rect, pos, Vector2(1, 1), Vector2(0, 1), col); }
	void DrawCenteredSizedRotated(Rectangle uvs, Vector2 pos, Vector2 size, Vector2 up, ColorRGBA col = ColorRGBA());
	void DrawCenteredSizedRotated(SpriteTile uvs, Vector2 pos, Vector2 size, Vector2 up, ColorRGBA col = ColorRGBA()) { DrawCenteredSizedRotated(uvs.rect, pos, size, up, col); }
	void DrawCenteredScaledRotated(Rectangle uvs, Vector2 pos, Vector2 scale, Vector2 up, ColorRGBA col = ColorRGBA());
	void DrawCenteredScaledRotated(SpriteTile uvs, Vector2 pos, Vector2 scale, Vector2 up, ColorRGBA col = ColorRGBA()) { DrawCenteredScaledRotated(uvs.rect, pos, scale, up, col); }
	void End();
	void FinalDraw();
};