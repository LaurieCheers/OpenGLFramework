#pragma once

#define MaxBalls 20
#define MaxBoxes 20
#define MaxMovables 20
#define MaxLineShapes 10
#define MaxTemplates 10
#define MaxInGame 100
#define MaxBoxesPerMovable 5

typedef enum
{
	COL_None,
	COL_Pink,
	COL_Green,
	COL_Yellow,
	COL_Blue,
	COL_Brown,
} GameColor;

typedef enum
{
	SHAPE_rigidbody,
	SHAPE_movable,
	SHAPE_line,
	SHAPE_box,
} GameShapeType;

typedef enum
{
	DRAW_man,
	DRAW_star,
	DRAW_shapeOnly,
} GameDrawType;

typedef struct GameShape_s
{
	GameShapeType shapetype;
	GameDrawType drawtype;
	GameColor color;
	int numFramesSinceLastCollision;
	float x, y;
	float savedx, savedy;
	bool erased;
} GameShape;

typedef struct RigidBody_s
{
	GameShape shape;
	float vx, vy;
	float radius, drag, bounciness, rolldrag, gravity;
	bool canCollide;
	float targetWalkSpeed;
	float stickingDirX;
	float stickingDirY;
	float slidingDirX;
	float slidingDirY;
	int walkFrame;
	int numConsecutivePogos;
} RigidBody;

typedef struct BoundingBox_s
{
	GameShape shape;
	float minx, miny, maxx, maxy;
} BoundingBox;

typedef struct MovableSegment_s
{
	GameShape shape;
	BoundingBox touchableShape;
	BoundingBox movableRange;
	BoundingBox* solidBoxes[MaxBoxesPerMovable];
} MovableSegment;

typedef struct LineShape_s
{
	GameShape shape;
	float ax;
	float ay;
	float offsx;
	float offsy;
} LineShape;
