#include "Rectangle.h"
#include <vector>

class Sprite
{
public:
	Rectangle rect;

	explicit Sprite(Rectangle rect) : rect(rect) {}
};

class SpriteBatch
{
public:
	std::vector<Sprite> sprites;
	GLfloat* vertices;
	GLuint programID;
	GLuint vertexbuffer;
	GLuint vertexPosition_modelspaceID;
	int maxSprites;

public:
	explicit SpriteBatch(GLuint programID, const char* positionAttribute, int maxSprites);
	void Begin();
	void Draw(Rectangle rect);
	void End();
	void FinalDraw();
};