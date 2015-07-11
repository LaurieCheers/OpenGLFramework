#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include "SpriteBatch.h"
#include "WalkerGame.h"
#include "walker_core.h"

RigidBody balls[MaxBalls];
BoundingBox boxes[MaxBoxes];
MovableSegment movables[MaxMovables];

bool playing = true;
bool showingEditor = false;
bool showingObjects = false;
bool showingResize = false;

// debug
void dayAssert(bool b)
{
static 	int breakhere = 0;
breakhere++;
}

// utility functions
float lerp(float f, float a, float b)
{
	return a + (b - a)*f;
}

float max(float a, float b)
{
	return a>b ? a : b;
}

float min(float a, float b)
{
	return a<b ? a : b;
}

float clamp(float f, float fmin, float fmax)
{
	if (f < fmin)
		return fmin;
	else if (f > fmax)
		return fmax;
	else
		return f;
}

void clampP(float* f, float fmin, float fmax)
{
	*f = clamp(*f, fmin, fmax);
}

// 2d vector maths
float vecLength(float x, float y)
{
	return sqrt(x*x + y*y);
}

float vecLengthSqr(float x, float y)
{
	return x*x + y*y;
}

float vecNormalize(float* x, float* y)
{
	float len = vecLength(*x, *y);
	*x /= len;
	*y /= len;
	return len;
}

float vecDot(float ax, float ay, float bx, float by)
{
	return ax*bx + ay*by;
}

float vecComponentAlongAxis(float vecx, float vecy, float axisx, float axisy)
{
	return vecDot(vecx, vecy, axisx, axisy);
}

float vecComponentAcrossAxis(float vecx, float vecy, float axisx, float axisy)
{
	return vecDot(vecx, vecy, axisy, -axisx);
}

void vecReframeToAxis(float vecx, float vecy, float axisx, float axisy, float* along, float* across)
{
	*along = vecDot(vecx, vecy, axisx, axisy);
	*across = vecDot(vecx, vecy, axisy, -axisx);
}

void vecReframeToWorld(float along, float across, float axisx, float axisy, float* resultx, float* resulty)
{
	*resultx = axisx * along + axisy * across;
	*resulty = axisy * along - axisx * across;
}

void vecReflect(float* x, float* y, float normx, float normy)
{
	float len = vecNormalize(x, y);
	float cosTheta = vecDot(*x, *y, normx, normy);
	*x -= normx * cosTheta * 2;
	*y -= normy * cosTheta * 2;
	*x *= len;
	*y *= len;
}

void vecClamp(float* x, float* y, float maxLength)
{
	float len = vecLength(*x, *y);

	if (len > maxLength)
	{
		vecNormalize(x, y);
		*x *= maxLength;
		*y *= maxLength;
	}
}

float clawTargetX = 0;
float clawTargetY = 0;
float clawPushStrength = 0.5;
float clawPushMaxStrength = 5;
bool clawSticky = 0;
bool clawStuck = 0;

float cloudPosX = 285;
float cloudPosY = 25;

float walkAccel = 0.8f;
//float targetWalkSpeed = 1;
float moonBootSuctionStrength = 3.0;
float unstickStrength = 0.1f;
float pogoStickStrength = 2.0;
float pogoStickDoubleStrength = 3.0;
float walkAnywhereFactor = 1; // can go up walls, fall off underhangs
float walkUpWallsFactor = 0.75; // can go up walls, fall off underhangs
float walkOnFloorsFactor = -0.01f; // can go up slopes, turn round at walls

float maxMoveSpeed = 1000;
int freezeWhenHitDuration = 5;

float cameraPosX = 0;
float cameraPosY = 0;

float gravityDirX = 0;
float gravityDirY = 1;

int numBoxes = 0;
int numBalls = 0;
int numMovables = 0;
int numLines = 0;

LineShape lines[MaxLineShapes];

GameShape* templates[MaxTemplates];
GameShape* inGame[MaxInGame];
int numTemplates = 0;
int numInGame = 0;

// the position you have grabbed on an object (relative to its basex/basey)
GameShape* touchingObject = 0;
GameShape* lastTouchedObject = 0;

typedef enum
{
	TOUCH_normal,
	TOUCH_cloud,
	TOUCH_handleTL,
	TOUCH_handleTR,
	TOUCH_handleBL,
	TOUCH_handleBR,
} TouchType;
TouchType touchingType = TOUCH_normal;
// the screen pos you are currently touching
float touchingPosX;
float touchingPosY;
float handledTouchUpToX;
float handledTouchUpToY;

//int numBoxes= sizeof(boxes) / sizeof(BoundingBox);
//int numMovables = sizeof(movables) / sizeof(MovableSegment);

void drawObject(GameShape* shape);

/*void setDrawGameColor(GameColor color, float alpha)
{
	switch (color)
	{
	case COL_None: glColor4f(1, 1, 1, alpha); break;
	case COL_Pink: glColor4f(1, 0.5, 0.5, alpha); break;
	case COL_Green: glColor4f(0, 1, 0, alpha); break;
	case COL_Yellow: glColor4f(1, 1, 0, alpha); break;
	case COL_Blue: glColor4f(0.5, 0.5, 1, alpha); break;
	case COL_Brown: glColor4f(0.7, 0.5, 0.4, alpha); break;
	}
}*/

void addTemplate(GameShape* shape)
{
	templates[numTemplates] = shape;
	numTemplates++;
}

void addInGame(GameShape* shape)
{
	inGame[numInGame] = shape;
	numInGame++;
}

MovableSegment* castToMovable(GameShape* shape)
{
	if (shape->shapetype == SHAPE_movable)
		return (MovableSegment*)shape;
	else
		return nullptr;
}

RigidBody* castToRb(GameShape* shape)
{
	if (shape->shapetype == SHAPE_rigidbody)
		return (RigidBody*)shape;
	else
		return nullptr;
}

BoundingBox* castToBox(GameShape* shape)
{
	if (shape->shapetype == SHAPE_box)
		return (BoundingBox*)shape;
	else
		return nullptr;
}

GameShape* rbinit(float x, float y, float radius, float drag, float bounciness, float rolldrag, float gravity, GameColor color, float walkspeed)
{
	assert(numBalls < MaxBalls);

	RigidBody* rb = &balls[numBalls];
	rb->shape.x = x;
	rb->shape.y = y;
	rb->shape.savedx = x;
	rb->shape.savedy = y;
	rb->vx = 0;
	rb->vy = 0;
	rb->radius = radius;
	rb->rolldrag = rolldrag;
	rb->drag = drag;
	rb->bounciness = bounciness;
	rb->gravity = gravity;
	rb->canCollide = true;
	rb->targetWalkSpeed = walkspeed;
	rb->walkFrame = 0;
	rb->stickingDirX = 0;
	rb->stickingDirY = 0;
	rb->slidingDirX = 0;
	rb->slidingDirY = 0;

	rb->shape.numFramesSinceLastCollision = 100;
	rb->shape.color = color;
	rb->shape.shapetype = SHAPE_rigidbody;
	rb->shape.drawtype = DRAW_man;
	numBalls++;

	return &rb->shape;
}

GameShape* starInit(float x, float y)
{
	GameShape* shape = rbinit(x, y, 20, 0, 0, 0, 0, COL_Yellow, 0);
	shape->drawtype = DRAW_star;
	return shape;
}

float componentInDirection(float ax, float ay, float bx, float by)
{
	vecNormalize(&bx, &by);
	return vecDot(ax, ay, bx, by);
}

void reframeToAxis(float vecx, float vecy, float axisx, float axisy, float* along, float* across)
{
	*along = vecDot(vecx, vecy, axisx, axisy);
	*across = vecDot(vecx, vecy, axisy, -axisx);
}

void reframeToWorld(float along, float across, float axisx, float axisy, float* resultx, float* resulty)
{
	*resultx = axisx * along + axisy * across;
	*resulty = axisy * along - axisx * across;
}

float normalizedComponentInDirection(float ax, float ay, float bx, float by)
{
	float alen = vecLength(ax, ay);
	if (alen == 0) return 0;
	float blen = vecLength(bx, by);
	if (blen == 0) return 0;

	float ascale = 1 / alen;
	float bscale = 1 / blen;
	return vecDot(ax*ascale, ay*ascale, bx*bscale, by*bscale) * ascale / bscale;
}

float shortestDistFromLineToPoint(float ax, float ay, float bx, float by, float px, float py)
{
	float component = normalizedComponentInDirection(bx - ax, by - ay, px - ax, py - ay);
	float x = (ax - px) + (bx - ax)*component;
	float y = (ay - py) + (by - ay)*component;
	return vecLength(x, y);
}

bool boxContainsPoint(BoundingBox* box, float px, float py, float tolerance)
{
	return box->minx < px + tolerance && box->miny < py + tolerance && box->maxx > px - tolerance && box->maxy > py - tolerance;
}

bool rbContainsPoint(RigidBody* rb, float px, float py, float tolerance)
{
	return vecLength(rb->shape.x - px, rb->shape.y - py) <= rb->radius + tolerance;
}

bool boxIntersectsRb(BoundingBox* box, RigidBody* rb)
{
	if (box->minx < rb->shape.x && box->maxx > rb->shape.x &&
		box->miny - rb->radius < rb->shape.y && box->maxy + rb->radius > rb->shape.y)
	{
		// within the box, allowing for +radius on the top and bottom
		return true;
	}
	else if (box->miny < rb->shape.y && box->maxy > rb->shape.y &&
		box->minx - rb->radius < rb->shape.x && box->maxx + rb->radius > rb->shape.x)
	{
		// within the box, allowing for +radius on the left and right
		return true;
	}
	else
	{
		// could still hit the corners
		return rbContainsPoint(rb, box->minx, box->miny, 0) ||
			rbContainsPoint(rb, box->maxx, box->miny, 0) ||
			rbContainsPoint(rb, box->maxx, box->maxy, 0) ||
			rbContainsPoint(rb, box->minx, box->maxy, 0);
	}
}

bool boxIntersectsBox(BoundingBox* box, BoundingBox* box2)
{
	return box->minx < box2->maxx && box2->minx < box->maxx &&
		box->miny < box2->maxy && box2->miny < box->maxy;
}

bool shapeContains(GameShape* shape, float x, float y, float tolerance)
{
	switch (shape->shapetype)
	{
	case SHAPE_rigidbody:
		return rbContainsPoint((RigidBody*)shape, x, y, tolerance);
	case SHAPE_box:
		return boxContainsPoint((BoundingBox*)shape, x, y, tolerance);
	case SHAPE_movable:
		return boxContainsPoint(&((MovableSegment*)shape)->touchableShape, x, y, tolerance);
	}
	return false;
}

void boxExpandToFit(BoundingBox* box, BoundingBox* toFit)
{
	if (box->minx > toFit->minx)
		box->minx = toFit->minx;
	if (box->miny > toFit->miny)
		box->miny = toFit->miny;
	if (box->maxx < toFit->maxx)
		box->maxx = toFit->maxx;
	if (box->maxy < toFit->maxy)
		box->maxy = toFit->maxy;

	box->shape.x = (box->minx + box->maxx) / 2;
	box->shape.y = (box->miny + box->maxy) / 2;
	box->shape.savedx = box->shape.x;
	box->shape.savedy = box->shape.y;
}

// clamp to 0 or more
float negZero(float f){ return (f > 0) ? f : 0; }

void moveBoxBy(BoundingBox* box, float movex, float movey)
{
	box->minx += movex;
	box->miny += movey;
	box->maxx += movex;
	box->maxy += movey;
	box->shape.x += movex;
	box->shape.y += movey;
}

void moveRbBy(RigidBody* rb, float movex, float movey)
{
	rb->shape.x += movex;
	rb->shape.y += movey;
}

void moveMovableBy(MovableSegment* movable, float movex, float movey)
{
	movable->shape.x += movex;
	movable->shape.y += movey;
	moveBoxBy(&movable->touchableShape, movex, movey);

	for (int Idx = 0; Idx < MaxBoxesPerMovable; Idx++)
	{
		BoundingBox* box = movable->solidBoxes[Idx];
		if (box)
		{
			moveBoxBy(box, movex, movey);
		}
	}
}

void moveObjectBy(GameShape* shape, float movex, float movey)
{
	switch (shape->shapetype)
	{
	case SHAPE_movable: moveMovableBy((MovableSegment*)shape, movex, movey); break;
	case SHAPE_rigidbody: moveRbBy((RigidBody*)shape, movex, movey); break;
	case SHAPE_box: moveBoxBy((BoundingBox*)shape, movex, movey); break;
	}
}

void resizeBoxBy(BoundingBox* box, float movex, float movey)
{
	box->maxx += movex;
	box->maxy += movey;
	box->shape.x = (box->minx + box->maxx) / 2;
	box->shape.y = (box->miny + box->maxy) / 2;
	box->shape.savedx = box->shape.x;
	box->shape.savedy = box->shape.y;
}

void resizeBoxWithin(BoundingBox* box, BoundingBox* container, float movex, float movey)
{
	float scalex = movex / (container->maxx - container->minx);
	float scaley = movey / (container->maxy - container->miny);
	moveBoxBy(box, scalex * (box->minx - container->minx), scaley * (box->miny - container->miny));
	resizeBoxBy(box, scalex * (box->maxx - box->minx), scaley * (box->maxy - box->miny));
}

void clampResize(BoundingBox* box, float* movex, float* movey)
{
	if (box->maxx + *movex < (box->minx + 1))
		*movex = (box->maxx - (box->minx + 1));

	if (box->maxy + *movey < (box->miny + 1))
		*movey = (box->maxy - (box->miny + 1));
}

void resizeObjectBRBy(GameShape* shape, float movex, float movey)
{
	switch (shape->shapetype)
	{
	case SHAPE_movable:
	{
		MovableSegment* mov = (MovableSegment*)shape;
		clampResize(&mov->touchableShape, &movex, &movey);
		for (int Idx = 0; Idx < MaxBoxesPerMovable; Idx++)
		{
			if (mov->solidBoxes[Idx])
				resizeBoxWithin(mov->solidBoxes[Idx], &mov->touchableShape, movex, movey);
		}
		resizeBoxBy(&mov->touchableShape, movex, movey);
		mov->shape.x = mov->touchableShape.shape.x;
		mov->shape.y = mov->touchableShape.shape.y;
		mov->shape.savedx = mov->touchableShape.shape.savedx;
		mov->shape.savedy = mov->touchableShape.shape.savedy;
	}
	break;

	case SHAPE_box:
		clampResize((BoundingBox*)shape, &movex, &movey);
		resizeBoxBy((BoundingBox*)shape, movex, movey);
		break;
	}
}

bool canResize(GameShape* shape)
{
	if (shape && shape->shapetype == SHAPE_movable && !shape->erased)
		return true;
	else
		return false;
}

void saveEditorState(GameShape* shape)
{
	shape->savedx = shape->x;
	shape->savedy = shape->y;
}

void loadRbEditorState(RigidBody* rb)
{
	rb->shape.x = rb->shape.savedx;
	rb->shape.y = rb->shape.savedy;
	rb->vx = 0;
	rb->vy = 0;
	rb->targetWalkSpeed = fabs(rb->targetWalkSpeed);
	rb->walkFrame = 0;
	rb->slidingDirX = 0;
	rb->slidingDirY = 0;
	rb->stickingDirX = 0;
	rb->stickingDirY = 0;
	rb->numConsecutivePogos = 0;
}

void loadMovableEditorState(MovableSegment* mov)
{
	moveMovableBy(mov, mov->shape.savedx - mov->shape.x, mov->shape.savedy - mov->shape.y);
}

void revertAllToEditorState()
{
	for (int Idx = 0; Idx < numInGame; Idx++)
	{
		switch (inGame[Idx]->shapetype)
		{
		case SHAPE_rigidbody: loadRbEditorState((RigidBody*)inGame[Idx]); break;
		case SHAPE_movable: loadMovableEditorState((MovableSegment*)inGame[Idx]); break;
		}
	}
}


bool couldMoveBoxBy(BoundingBox* box, float movex, float movey)
{
	BoundingBox areaCovered;
	areaCovered.minx = box->minx - negZero(-movex);
	areaCovered.miny = box->miny - negZero(-movey);
	areaCovered.maxx = box->maxx + negZero(movex);
	areaCovered.maxy = box->maxy + negZero(movey);

	if (areaCovered.minx < 0 || areaCovered.miny < 0 || areaCovered.maxx > 320 || areaCovered.maxy > 480)
		return false;

	for (int Idx = 0; Idx < numBalls; Idx++)
	{
		if (boxIntersectsRb(&areaCovered, &balls[Idx]))
			return false;
	}
	//	for( int Idx = 0; Idx < numBoxes; Idx++ )
	//	{
	//		if( boxes[Idx].blocker && boxIntersectsBox(&areaCovered, &boxes[Idx]) )
	//		return false;
	//	}

	return true;
}

GameShape* lineShapeInit(float x1, float y1, float x2, float y2, GameColor color)
{
	assert(numLines < MaxLineShapes);

	LineShape* line = &lines[numLines];
	line->ax = x1;
	line->ay = y1;
	line->offsx = x2 - x1;
	line->offsy = y2 - y1;
	line->shape.color = color;
	line->shape.shapetype = SHAPE_line;
	line->shape.drawtype = DRAW_shapeOnly;
	numLines++;
	return &line->shape;
}

GameShape* movableInit5(float rangeLeft, float rangeRight, float rangeUp, float rangeDown, GameShape* box1, GameShape* box2, GameShape* box3, GameShape* box4, GameShape* box5)
{
	assert(numMovables < MaxMovables);

	MovableSegment* mov = &movables[numMovables];
	mov->solidBoxes[0] = (BoundingBox*)box1;
	mov->solidBoxes[1] = (BoundingBox*)box2;
	mov->solidBoxes[2] = (BoundingBox*)box3;
	mov->solidBoxes[3] = (BoundingBox*)box4;
	mov->solidBoxes[4] = (BoundingBox*)box5;

	// copy the first box
	mov->touchableShape = *mov->solidBoxes[0];
	mov->shape.numFramesSinceLastCollision = 1000;
	mov->shape.shapetype = SHAPE_movable;
	mov->shape.drawtype = DRAW_shapeOnly;
	mov->shape.color = box1->color;
	for (int Idx = 1; Idx < MaxBoxesPerMovable; Idx++)
	{
		if (mov->solidBoxes[Idx])
			boxExpandToFit(&mov->touchableShape, mov->solidBoxes[Idx]);
	}

	mov->shape.x = mov->touchableShape.shape.x;
	mov->shape.y = mov->touchableShape.shape.y;
	mov->shape.savedx = mov->touchableShape.shape.savedx;
	mov->shape.savedy = mov->touchableShape.shape.savedy;

	mov->movableRange.minx = mov->touchableShape.minx - rangeLeft;
	mov->movableRange.miny = mov->touchableShape.miny - rangeUp;
	mov->movableRange.maxx = mov->touchableShape.maxx + rangeRight;
	mov->movableRange.maxy = mov->touchableShape.maxy + rangeDown;

	float minTouchableSize = 40;
	float width = mov->movableRange.maxx - mov->movableRange.minx;
	if (width < minTouchableSize)
	{
		mov->movableRange.minx -= (minTouchableSize - width) / 2;
		mov->movableRange.maxx += (minTouchableSize - width) / 2;
	}

	float height = mov->movableRange.maxy - mov->movableRange.miny;
	if (height < minTouchableSize)
	{
		mov->movableRange.miny -= (minTouchableSize - height) / 2;
		mov->movableRange.maxy += (minTouchableSize - height) / 2;
	}

	mov->shape.savedx = 0.5f*(mov->touchableShape.minx + mov->touchableShape.maxx);
	mov->shape.savedy = 0.5f*(mov->touchableShape.miny + mov->touchableShape.maxy);

	numMovables++;
	return &mov->shape;
}

GameShape* movableInit2(float rangeLeft, float rangeRight, float rangeUp, float rangeDown, GameShape* box1, GameShape* box2)
{
	return movableInit5(rangeLeft, rangeRight, rangeUp, rangeDown, box1, box2, nullptr, nullptr, nullptr);
}

GameShape* movableInit(float rangeLeft, float rangeRight, float rangeUp, float rangeDown, GameShape* box1)
{
	return movableInit5(rangeLeft, rangeRight, rangeUp, rangeDown, box1, nullptr, nullptr, nullptr, nullptr);
}

GameShape* boxInit(float minx, float miny, float maxx, float maxy, GameColor color)
{
	assert(numBoxes < MaxBoxes);
	BoundingBox* box = &boxes[numBoxes];
	box->minx = minx;
	box->miny = miny;
	box->maxx = maxx;
	box->maxy = maxy;
	box->shape.x = (box->minx + box->maxx) / 2;
	box->shape.y = (box->miny + box->maxy) / 2;
	box->shape.savedx = box->shape.x;
	box->shape.savedy = box->shape.y;
	box->shape.color = color;
	box->shape.shapetype = SHAPE_box;
	box->shape.drawtype = DRAW_shapeOnly;
	numBoxes++;

	return &box->shape;
}

void initGameObjects()
{
	//		rbinit(&blob,	-25,	-100,	30,		0,		1.0,		0.1);
	//		rbinit(&blob,	50,	50,	15,		0,		0.05,		0.15);
	//	x,	y,	radius,	drag,	bounciness,	rolldrag, gravity,  color, walkspeed
	addInGame(rbinit(100, 150, 15, 0, 0.05f, 1, 0.05f, COL_Blue, 0.95f));
	addInGame(rbinit(130, 150, 15, 0, 0.05f, 1, 0.05f, COL_Blue, 1.05f));
	addInGame(rbinit(170, 150, 15, 0, 0.05f, 1, 0.05f, COL_Pink, 1.0f));
	addInGame(rbinit(290, 150, 15, 0, 0.05f, 1, 0.05f, COL_Pink, 0.9f));
	//		rbinit(&claw, 100, 100, 25, 0, 0.0, 0.1);
	//		blob.vx = 2;

	addInGame(boxInit(-20, 0, 0, 480, COL_Green));
	addInGame(boxInit(-20, -20, 340, 0, COL_Green));
	addInGame(boxInit(320, 0, 340, 480, COL_Green));
	addInGame(boxInit(-20, 480, 340, 500, COL_Green));

	float blockLHS = 200;
	float blockRHS = 260;
//	addInGame(movableInit(135.f, 35.f, 0.f, 0.f, boxInit(blockLHS, 25, blockRHS, 95, COL_Yellow)));
//	addInGame(movableInit(135.f, 35.f, 0.f, 0.f, boxInit(blockLHS, 100, blockRHS, 160, COL_Yellow)));
//	addInGame(movableInit(135.f, 35.f, 0.f, 0.f, boxInit(blockLHS, 170, blockRHS, 240, COL_Yellow)));

	addInGame(movableInit(135.f, 35.f, 0.f, 0.f, boxInit(blockLHS, 420, blockRHS, 430, COL_Brown)));

	addInGame(lineShapeInit(100, 250, 160, 270, COL_Yellow));
	addInGame(lineShapeInit(160, 270, 180, 330, COL_Yellow));

	addInGame(lineShapeInit(320, 250, 240, 270, COL_Yellow));
	addInGame(lineShapeInit(240, 270, 220, 330, COL_Yellow));
}

MovableSegment* getMovableWithBox(BoundingBox* box)
{
	for (int MIdx = 0; MIdx < numMovables; MIdx++)
	{
		for (int BIdx = 0; BIdx < MaxBoxesPerMovable; BIdx++)
		{
			if (movables[MIdx].solidBoxes[BIdx] == box)
				return &movables[MIdx];
		}
	}

	return nullptr;
}

float findcollision_rb_point(float timeStep, RigidBody* rb, float px, float py, float* hitnormalx, float* hitnormaly)
{
	const float EPSILON = 0.0001f;
	float x1 = rb->shape.x - rb->vx * EPSILON;
	float y1 = rb->shape.y - rb->vy * EPSILON;
	float x2 = rb->shape.x + rb->vx * (timeStep + EPSILON);
	float y2 = rb->shape.y + rb->vy * (timeStep + EPSILON);

	float pureComponent = normalizedComponentInDirection(x2 - x1, y2 - y1, px - x1, py - y1);

	float component;
	if (pureComponent > 1)
		component = 1;
	else if (pureComponent < 0)
		component = 0;
	else
		component = pureComponent;

	float nearestx = x1 + (x2 - x1)*component;
	float nearesty = y1 + (y2 - y1)*component;
	float nearestDist = vecLength(nearestx - px, nearesty - py);

	if (nearestDist < rb->radius)
	{
		// get the theoretical nearest position along this line (not necessarily reached this timestep)
		float pureNearestx = x1 + (x2 - x1)*pureComponent;
		float pureNearesty = y1 + (y2 - y1)*pureComponent;
		float pureNearestDist = vecLength(pureNearestx - px, pureNearesty - py);

		// now we have to solve for the first point along x1y1->x2y2 that's rb->radius away from pxpy
		float distFromNearest = sqrt(rb->radius*rb->radius - pureNearestDist*pureNearestDist);
		float fraction = distFromNearest / vecLength((x2 - x1), (y2 - y1));

		float collisionx = pureNearestx - (x2 - x1)*fraction;
		float collisiony = pureNearesty - (y2 - y1)*fraction;

		*hitnormalx = collisionx - px;
		*hitnormaly = collisiony - py;
		vecNormalize(hitnormalx, hitnormaly);

		float collisionDist = vecLength(collisionx - x1, collisiony - y1);
		float moveDist = vecLength(x2 - x1, y2 - y1);
		float collisionTime = timeStep * collisionDist / moveDist;
		if (collisionTime < 0)
			collisionTime = 0;
		else if (collisionTime > timeStep)
			collisionTime = timeStep;

		return collisionTime;
	}

	return timeStep;
}

float findcollision_rb_lineshape(float timeStep, RigidBody* rb, LineShape* line, float* hitnormalx, float* hitnormaly)
{
	const float EPSILON = 0.0001f;
	float x1 = rb->shape.x - rb->vx * EPSILON;
	float y1 = rb->shape.y - rb->vy * EPSILON;
	float x2 = rb->shape.x + rb->vx * (timeStep + EPSILON);
	float y2 = rb->shape.y + rb->vy * (timeStep + EPSILON);

	// rb's distance from the polygon axis, before and after (visualising the axis as the equator)
	float latitudeBefore = componentInDirection(x1 - line->ax, y1 - line->ay, line->offsy, -line->offsx);
	float latitudeAfter = componentInDirection(x2 - line->ax, y2 - line->ay, line->offsy, -line->offsx);
	float normx, normy;

	float collisionFraction = 1;
	bool collPos = (latitudeBefore > rb->radius && latitudeAfter < rb->radius);
	bool collNeg = (latitudeBefore < -rb->radius && latitudeAfter > -rb->radius);

	if (collPos)
	{
		// crossing the positive side
		collisionFraction = (rb->radius - latitudeBefore) / (latitudeAfter - latitudeBefore);
		normx = line->offsy;
		normy = -line->offsx;
	}
	else if (collNeg)
	{
		// crossing the negative side
		collisionFraction = (-rb->radius - latitudeBefore) / (latitudeAfter - latitudeBefore);
		normx = -line->offsy;
		normy = line->offsx;
	}

	if (collPos || collNeg)
	{
		float collisionX = x1 + (x2 - x1)*collisionFraction;
		float collisionY = y1 + (y2 - y1)*collisionFraction;

		float collisionLongitude = componentInDirection(collisionX - line->ax, collisionY - line->ay, line->offsx, line->offsy);

		if (collisionLongitude >= 0 && (collisionLongitude*collisionLongitude <= vecLengthSqr(line->offsx, line->offsy)))
		{
			// we have our collision!
			*hitnormalx = normx;
			*hitnormaly = normy;
			vecNormalize(hitnormalx, hitnormaly);

			// now calculate the real collision time (collisionFraction was in terms of the padded trajectory)
			if (fabs(rb->vx) > fabs(rb->vy))
				return (collisionX - rb->shape.x) / rb->vx;
			else
				return (collisionY - rb->shape.y) / rb->vy;
		}
	}

	float resultTime;

	resultTime = findcollision_rb_point(timeStep, rb, line->ax, line->ay, hitnormalx, hitnormaly);
	if (resultTime < timeStep) return resultTime;
	resultTime = findcollision_rb_point(timeStep, rb, line->ax + line->offsx, line->ay + line->offsy, hitnormalx, hitnormaly);
	if (resultTime < timeStep) return resultTime;

	return timeStep;
}

float findcollision_rb_box(float timeStep, RigidBody* rb, BoundingBox* box, float* hitnormalx, float* hitnormaly)
{
	const float EPSILON = 0.0001f;
	float x1 = rb->shape.x - rb->vx * EPSILON;
	float y1 = rb->shape.y - rb->vy * EPSILON;
	float x2 = rb->shape.x + rb->vx * (timeStep + EPSILON);
	float y2 = rb->shape.y + rb->vy * (timeStep + EPSILON);

	// assume we'll return the whole fraction, i.e. we complete the whole time step without collisions
	// this handles two possible collisions at once: one with the top/bottom and one with the side
	float vtime = timeStep;
	float htime = timeStep;
	float vdirx = 0; // normal of the wall we hit
	float hdiry = 0;

	float paddedminx = box->minx - rb->radius;
	if (x1 <= paddedminx && x2 > paddedminx)
	{
		// entering lhs
		vtime = (paddedminx - rb->shape.x) / rb->vx;
		vdirx = -1;
	}
	else
	{
		float paddedmaxx = box->maxx + rb->radius;
		if (x1 >= paddedmaxx && x2 < paddedmaxx)
		{
			// entering rhs
			vtime = (paddedmaxx - rb->shape.x) / rb->vx;
			vdirx = 1;
		}
	}

	float paddedminy = box->miny - rb->radius;
	if (y1 <= paddedminy && y2 > paddedminy)
	{
		// entering top
		htime = (paddedminy - rb->shape.y) / rb->vy;
		hdiry = -1;
	}
	else
	{
		float paddedmaxy = box->maxy + rb->radius;
		if (y1 >= paddedmaxy && y2 < paddedmaxy)
		{
			// entering bot
			htime = (paddedmaxy - rb->shape.y) / rb->vy;
			hdiry = 1;
		}
	}

	if (vtime < timeStep)
	{
		if (vtime <= 0)
			vtime = 0;

		float yc = rb->shape.y + rb->vy * vtime;
		if (yc < box->miny || yc > box->maxy)
		{
			vtime = timeStep;
			vdirx = 0;
		}
	}

	if (htime < timeStep)
	{
		if (htime <= 0)
			htime = 0;

		float xc = rb->shape.x + rb->vx * htime;
		if (xc < box->minx || xc > box->maxx)
		{
			htime = timeStep;
			hdiry = 0;
		}
	}

	if (vtime < htime)
	{
		*hitnormalx = vdirx;
		*hitnormaly = 0;
		return vtime;
	}
	else if (htime < timeStep)
	{
		*hitnormalx = 0;
		*hitnormaly = hdiry;
		return htime;
	}

	float resultTime;

	resultTime = findcollision_rb_point(timeStep, rb, box->minx, box->miny, hitnormalx, hitnormaly);
	if (resultTime < timeStep) return resultTime;
	resultTime = findcollision_rb_point(timeStep, rb, box->minx, box->maxy, hitnormalx, hitnormaly);
	if (resultTime < timeStep) return resultTime;
	resultTime = findcollision_rb_point(timeStep, rb, box->maxx, box->miny, hitnormalx, hitnormaly);
	if (resultTime < timeStep) return resultTime;
	resultTime = findcollision_rb_point(timeStep, rb, box->maxx, box->maxy, hitnormalx, hitnormaly);
	if (resultTime < timeStep) return resultTime;

	return timeStep;
}

void findcollision_rb_shape(RigidBody* rb, GameShape* shape, float* timeRemaining, GameShape** collidedWith, float* bestHitnormalx, float* bestHitnormaly)
{
	float hitnormalx;
	float hitnormaly;
	float collisionTime = *timeRemaining;

	switch (shape->shapetype)
	{
	case SHAPE_box:
		collisionTime = findcollision_rb_box(*timeRemaining, rb, (BoundingBox*)shape, &hitnormalx, &hitnormaly);
		break;
	case SHAPE_line:
		collisionTime = findcollision_rb_lineshape(*timeRemaining, rb, (LineShape*)shape, &hitnormalx, &hitnormaly);
		break;
	case SHAPE_movable:
	{
		MovableSegment* mov = (MovableSegment*)shape;
		for (int Idx = 0; Idx < MaxBoxesPerMovable; Idx++)
		{
			if (mov->solidBoxes[Idx])
			{
				findcollision_rb_shape(rb, &mov->solidBoxes[Idx]->shape, timeRemaining, collidedWith, bestHitnormalx, bestHitnormaly);
			}
		}
		break;
	}
	}
	if (collisionTime < *timeRemaining)//&& rb->shape.color != shape->color )
	{
		*timeRemaining = collisionTime;
		*collidedWith = shape;
		*bestHitnormalx = hitnormalx;
		*bestHitnormaly = hitnormaly;
	}
}

void WalkerGame::drawBoundingBox(BoundingBox* box, SpriteBatch* spriteBatch)
{
//C	daySetTile(&whiteTile);
//C	setDrawGameColor(box->shape.color, 1);
//C	dayDrawQuadXYXY(box->minx, box->miny, box->maxx, box->maxy);
	spriteBatch->Draw(whiteTile, Rectangle(box->minx, box->miny, box->maxx - box->minx, box->maxy-box->miny));
}

void WalkerGame::drawLineShape(LineShape* line, SpriteBatch* spriteBatch)
{
//C	daySetTile(&whiteTile);
//C	setDrawGameColor(line->shape.color, 1);
	float length = vecLength(line->offsx, line->offsy);
//C	dayDrawQuadRotatedSize(line->ax + line->offsx / 2, line->ay + line->offsy / 2, divisor * 3, 1, line->offsx, line->offsy);
	spriteBatch->DrawCenteredSizedRotated(whiteTile, Vector2(line->ax + line->offsx / 2, line->ay + line->offsy / 2), 
		Vector2(2, length), Vector2(line->offsx / length, line->offsy / length));
}

void WalkerGame::drawMovable(MovableSegment* mov, SpriteBatch* spriteBatch)
{
	float thickness;

	if (mov->shape.numFramesSinceLastCollision < freezeWhenHitDuration)
	{
		//CsetDrawGameColor(mov->solidBoxes[0]->shape.color, 0.0);
		thickness = 0;
	}
	else
	{
		//CsetDrawGameColor(mov->solidBoxes[0]->shape.color, 0.1);
		thickness = 3;
	}

	//CdayDrawQuadXYXY(mov->touchableShape.minx - thickness, mov->touchableShape.miny - thickness, mov->touchableShape.maxx + thickness, mov->touchableShape.maxy + thickness);
	float baseX = mov->touchableShape.minx - thickness;
	float baseY = mov->touchableShape.miny - thickness;
	spriteBatch->Draw(whiteTile, Rectangle(baseX, baseY, mov->touchableShape.maxx + thickness - baseX, mov->touchableShape.maxy + thickness - baseY));

	for (int Idx = 0; Idx < MaxBoxesPerMovable; Idx++)
	{
		if (mov->solidBoxes[Idx])
			drawBoundingBox(mov->solidBoxes[Idx], spriteBatch);
	}
}

void WalkerGame::drawStar(RigidBody* rb, SpriteBatch* spriteBatch)
{
//C	glColor4f(1, 1, 0, 1);
//C	daySetTransparent();
//C	daySetTile(&starTile);
//C	dayDrawQuadCentered(rb->shape.x, rb->shape.y);
	spriteBatch->DrawCentered(starTile, Vector2(rb->shape.x, rb->shape.y));
}

void WalkerGame::drawMan(RigidBody* rb, SpriteBatch* spriteBatch)
{
//TODO	daySetTransparent();

	SpriteTile spr;
	if (rb->walkFrame < 0)
	{
		//daySetTile(&fallTile);
		spr = fallTile;
	}
	else if ((rb->walkFrame / 5) % 2 == 0)
	{
		//daySetTile(&walk1Tile);
		spr = walk1Tile;
	}
	else
	{
		//daySetTile(&walk2Tile);
		spr = walk2Tile;
	}

	//TODO setDrawGameColor(rb->shape.color, 1);
	ColorRGBA testCol;
	testCol.red = 1.0f;
	testCol.green = 0.0f;
	testCol.blue = 0.0f;
	testCol.alpha = 1.0f;

	float wScale = rb->targetWalkSpeed > 0.0f ? 1.0f : -1.0f;
	float size = 0.7f;

	if (rb->stickingDirX || rb->stickingDirY)
	{
		//dayDrawQuadRotatedScale(rb->shape.x, rb->shape.y, wScale, 1, rb->stickingDirX*size, rb->stickingDirY*size);
		spriteBatch->DrawCenteredScaledRotated(spr, Vector2(rb->shape.x, rb->shape.y), Vector2(wScale, 1), Vector2(rb->stickingDirX*size, rb->stickingDirY*size), testCol);
	}
	else if (rb->slidingDirX || rb->slidingDirY)
	{
		spr = fallTile;
		//daySetTile(&fallTile);
		//dayDrawQuadRotatedScale(rb->shape.x, rb->shape.y, wScale, 1, rb->slidingDirX*size, rb->slidingDirY*size);
		spriteBatch->DrawCenteredScaledRotated(spr, Vector2(rb->shape.x, rb->shape.y), Vector2(wScale, 1), Vector2(rb->slidingDirX*size, rb->slidingDirY*size), testCol);
	}
	else
	{
		//dayDrawQuadRotatedScale(rb->shape.x, rb->shape.y, wScale, 1, gravityDirX*size, gravityDirY*size);
		spriteBatch->DrawCenteredScaledRotated(spr, Vector2(rb->shape.x, rb->shape.y), Vector2(wScale, 1), Vector2(gravityDirX*size, gravityDirY*size), testCol);
	}
}

void WalkerGame::drawObject(GameShape* shape, SpriteBatch* spriteBatch)
{
	switch (shape->drawtype)
	{
	case DRAW_man: drawMan((RigidBody*)shape, spriteBatch); return;
	case DRAW_star: drawStar((RigidBody*)shape, spriteBatch); return;
	case DRAW_shapeOnly:
		switch (shape->shapetype)
		{
		case SHAPE_box: drawBoundingBox((BoundingBox*)shape, spriteBatch); return;
		case SHAPE_line: drawLineShape((LineShape*)shape, spriteBatch); return;
		case SHAPE_movable: drawMovable((MovableSegment*)shape, spriteBatch); return;
		}
		return;
	}
}

float boxBoundingRadius(BoundingBox* box)
{
	float w = box->maxx - box->minx;
	float h = box->maxy - box->miny;
	if (w > h)
		return w / 2;
	else
		return h / 2;
}

float objectBoundingRadius(GameShape* shape)
{
	switch (shape->shapetype)
	{
	case SHAPE_box: return boxBoundingRadius((BoundingBox*)shape);
	case SHAPE_rigidbody: return ((RigidBody*)shape)->radius;
	case SHAPE_movable: return boxBoundingRadius(&((MovableSegment*)shape)->touchableShape);
	}
	return 10;
}

/*void drawObjectAt(GameShape* shape, float x, float y, float maxRadius)
{
	float radius = objectBoundingRadius(shape);

	glPushMatrix();
	glTranslatef(x, y, 0);
	if (radius > maxRadius)
		glScalef(maxRadius / radius, maxRadius / radius, 1);
	glTranslatef(-shape->x, -shape->y, 0);

	drawObject(shape);

	glPopMatrix();
}*/

GameShape* copyBox(BoundingBox* box)
{
	if (!box) return nullptr;

	return boxInit(box->minx, box->miny, box->maxx, box->maxy, box->shape.color);
}

GameShape* copyMovable(MovableSegment* mov)
{
	if (!mov) return nullptr;

	return movableInit5(0, 0, 0, 0,
		copyBox(mov->solidBoxes[0]),
		copyBox(mov->solidBoxes[1]),
		copyBox(mov->solidBoxes[2]),
		copyBox(mov->solidBoxes[3]),
		copyBox(mov->solidBoxes[4]));
}

GameShape* copyRb(RigidBody* rb)
{
	if (!rb) return nullptr;

	GameShape* result = rbinit(rb->shape.x, rb->shape.y, rb->radius, rb->drag, rb->bounciness, rb->rolldrag, rb->gravity, rb->shape.color, rb->targetWalkSpeed);
	result->drawtype = rb->shape.drawtype;
	return result;
}

GameShape* copyObject(GameShape* shape)
{
	if (!shape) return nullptr;

	if (shape->erased)
	{
		shape->erased = false;
		return shape;
	}

	switch (shape->shapetype)
	{
	case SHAPE_box: return copyBox((BoundingBox*)shape);
	case SHAPE_movable: return copyMovable((MovableSegment*)shape);
	case SHAPE_rigidbody: return copyRb((RigidBody*)shape);
	}
	return nullptr;
}

void eraseObject(GameShape* shape)
{
	if (!shape) return;

	shape->erased = true;

	int Idx = 0;
	for (; Idx < numInGame; Idx++)
	{
		if (inGame[Idx] == shape)
			break;
	}

	if (Idx < numInGame) // found it?
	{
		numInGame--;
		for (; Idx < numInGame; Idx++)
		{
			inGame[Idx] = inGame[Idx + 1];
		}
		inGame[numInGame] = nullptr;
	}
}

float stepTowards(float value, float target, float step)
{
	if (value < target)
	{
		value += step;
		if (value > target)
			value = target;
	}
	else if (value > target)
	{
		value -= step;
		if (value < target)
			value = target;
	}

	return value;
}

void rbtick(RigidBody* rb)
{
	float timeRemaining = 1;
	float moonBootGravityX = 0;
	float moonBootGravityY = 0;

	rb->shape.numFramesSinceLastCollision++;

	if (rb->shape.numFramesSinceLastCollision > 12)
	{
		if (rb->walkFrame >= 0 || rb->walkFrame <= -30)
			rb->walkFrame = -1; // falling
		else
			rb->walkFrame--;

		rb->stickingDirX = 0;
		rb->stickingDirY = 0;
		rb->slidingDirX = 0;
		rb->slidingDirY = 0;
	}
	else if (rb->walkFrame >= 30 || rb->walkFrame < 0)
	{
		rb->walkFrame = 0;
	}
	else
	{
		rb->walkFrame++;
	}

	if (!rb->canCollide)
	{
		rb->shape.x += rb->vx;
		rb->shape.y += rb->vy;
	}
	else
	{
		while (timeRemaining > 0)
		{
			GameShape* firstCollisionShape = nullptr;
			float firstCollisionTime = timeRemaining;
			float firstHitnormalx;
			float firstHitnormaly;
			for (int Idx = 0; Idx < numInGame; Idx++)
			{
				findcollision_rb_shape(rb, inGame[Idx], &firstCollisionTime, &firstCollisionShape, &firstHitnormalx, &firstHitnormaly);
			}

			rb->shape.x += rb->vx * firstCollisionTime;
			rb->shape.y += rb->vy * firstCollisionTime;
			if (firstCollisionTime < timeRemaining)
			{
				float oldvx = rb->vx;
				float oldvy = rb->vy;

				float bouncingSpeed;
				float rollingSpeed;
				reframeToAxis(rb->vx, rb->vy, firstHitnormalx, firstHitnormaly, &bouncingSpeed, &rollingSpeed);
				bouncingSpeed *= rb->bounciness * -1;
				rollingSpeed *= rb->rolldrag;
				reframeToWorld(bouncingSpeed, rollingSpeed, firstHitnormalx, firstHitnormaly, &rb->vx, &rb->vy);

				float walkSpeedX = rb->vx;
				float walkSpeedY = rb->vy;

				// cos(angle) of this wall with respect to gravity
				float cosSlope = componentInDirection(gravityDirX, gravityDirY, firstHitnormalx, firstHitnormaly);

				if (rb->shape.numFramesSinceLastCollision > 6)
				{
					// pick a new walk direction based on landing velocity
					if (componentInDirection(rb->vy, -rb->vx, firstHitnormalx, firstHitnormaly) < 0)
						rb->targetWalkSpeed = -fabs(rb->targetWalkSpeed);
					else
						rb->targetWalkSpeed = fabs(rb->targetWalkSpeed);
				}

				if (cosSlope <= -0.9)
				{
					// walk along this floor
					// push away from the wall a little, try to come unstuck
					//moonBootGravityX = firstHitnormalx * unstickStrength;
					//moonBootGravityY = firstHitnormaly * unstickStrength;
					if (cosSlope < -0.9f && firstCollisionShape->color == COL_Brown)
					{
						rb->numConsecutivePogos++;
						moonBootGravityX = firstHitnormalx * pogoStickStrength * rb->numConsecutivePogos;
						moonBootGravityY = firstHitnormaly * pogoStickStrength * rb->numConsecutivePogos;
					}

					walkSpeedX = -firstHitnormaly * rb->targetWalkSpeed;
					walkSpeedY = firstHitnormalx * rb->targetWalkSpeed;

					float sinSlope = componentInDirection(gravityDirX, gravityDirY, walkSpeedX, walkSpeedY);
					float slopeSpeedFactor = 1 + sinSlope * 2;
					walkSpeedX *= slopeSpeedFactor;
					walkSpeedY *= slopeSpeedFactor;

					float vc = componentInDirection(oldvx, oldvy, firstHitnormalx, firstHitnormaly);
					if (vc < -0.3 && cosSlope > -0.1)
					{
						rb->targetWalkSpeed = -rb->targetWalkSpeed;
						walkSpeedX = -walkSpeedX;
						walkSpeedY = -walkSpeedY;
					}

					rb->stickingDirX = 0;
					rb->stickingDirY = 0;
					rb->slidingDirX = 0;
					rb->slidingDirY = 0;
				}
				else if (firstCollisionShape && firstCollisionShape->color == rb->shape.color && cosSlope <= walkUpWallsFactor)
				{
					// moon boots: accelerate to walking pace along this wall
					// push into the wall a little to make sure we keep contact
					moonBootGravityX = -firstHitnormalx * moonBootSuctionStrength;
					moonBootGravityY = -firstHitnormaly * moonBootSuctionStrength;
					rb->numConsecutivePogos = 0;

					if (cosSlope <= walkUpWallsFactor)
					{
						walkSpeedX = -firstHitnormaly * rb->targetWalkSpeed;
						walkSpeedY = firstHitnormalx * rb->targetWalkSpeed;
					}
					else
					{
						walkSpeedX = 0;
						walkSpeedY = 0;
						rb->walkFrame = 0;
					}

					rb->stickingDirX = -firstHitnormalx;
					rb->stickingDirY = -firstHitnormaly;
					rb->slidingDirX = 0;
					rb->slidingDirY = 0;
				}
				else
				{
					// slide down this steep surface
					rb->slidingDirX = -firstHitnormaly;
					rb->slidingDirY = firstHitnormalx;
					rb->numConsecutivePogos = 0;

					float upward = componentInDirection(firstHitnormaly, -firstHitnormalx, gravityDirX, gravityDirY);
					if (upward > 0)
					{
						rb->targetWalkSpeed = -fabs(rb->targetWalkSpeed);
						rb->slidingDirX = -rb->slidingDirX;
						rb->slidingDirY = -rb->slidingDirY;
					}
					else
					{
						rb->targetWalkSpeed = fabs(rb->targetWalkSpeed);
					}

					float speed = vecLength(rb->vx, rb->vy);
					speed = speed < 1 ? 1 : speed;
					walkSpeedX = -firstHitnormaly * speed;
					walkSpeedY = firstHitnormalx * speed;
					if (rb->targetWalkSpeed < 0)
					{
						walkSpeedX = -walkSpeedX;
						walkSpeedY = -walkSpeedY;
					}

					rb->stickingDirX = 0;
					rb->stickingDirY = 0;
				}

				float steppedvx = stepTowards(rb->vx, walkSpeedX, walkAccel);
				float steppedvy = stepTowards(rb->vy, walkSpeedY, walkAccel);

				// make sure we're never left moving into the wall (else we can pass through)
				//				float vout = componentInDirection(steppedvx,steppedvy, firstHitnormalx, firstHitnormaly);
				//				float newvx = steppedvx;
				//				float newvy = steppedvy;
				//				if ( vout < 0 )
				//				{
				//					// 1.0001 to avoid rounding errors
				//					newvx -= firstHitnormalx * vout * 1.0001;
				//					newvy -= firstHitnormaly * vout * 1.0001;
				//				}
				//				rb->vx = newvx;
				//				rb->vy = newvy;
				float along, across;
				reframeToAxis(steppedvx, steppedvy, firstHitnormalx, firstHitnormaly, &along, &across);
				if (along < 0.001f)
					along = 0.001f;
				reframeToWorld(along, across, firstHitnormalx, firstHitnormaly, &rb->vx, &rb->vy);

				rb->shape.numFramesSinceLastCollision = 0;

				if (firstCollisionShape && firstCollisionShape->shapetype == SHAPE_box)
				{
					MovableSegment* mov = getMovableWithBox((BoundingBox*)firstCollisionShape);
					if (mov)
					{
						mov->shape.numFramesSinceLastCollision = 0;
					}
				}
			}

			// make sure we're always taking off _some_ time
			if (firstCollisionTime > 0.001)
				timeRemaining -= firstCollisionTime;
			else
				timeRemaining -= 0.001f;
		}
	}

	rb->vx *= (1 - rb->drag);
	rb->vy *= (1 - rb->drag);
	rb->vx += rb->gravity * gravityDirX;
	rb->vy += rb->gravity * gravityDirY;

	rb->vx += moonBootGravityX;
	rb->vy += moonBootGravityY;
}

void shapeTick(GameShape* shape)
{
	shape->numFramesSinceLastCollision++;

	if (shape->shapetype == SHAPE_rigidbody)
	{
		rbtick((RigidBody*)shape);
	}
}

#define kFilteringFactor 0.1

/*- (void)accelerometer:(UIAccelerometer *)accelerometer didAccelerate : (UIAcceleration *)acceleration
{
	// low pass filter
	//	accelX = (acceleration.x * kFilteringFactor) + (accelX * (1.0 - kFilteringFactor));
	//  accelY = (acceleration.y * kFilteringFactor) + (accelY * (1.0 - kFilteringFactor));
	//accelZ = (acceleration.z * kFilteringFactor) + (accelZ * (1.0 - kFilteringFactor));	
	accelX = acceleration.x;
	accelY = acceleration.y;
	accelZ = acceleration.z;

	float oldGravityDirX = gravityDirX;
	float oldGravityDirY = gravityDirY;

	if (vecLength(accelX, accelY) > 0.2)
	{
		gravityDirX = accelX;
		gravityDirY = -accelY;
		vecNormalize(&gravityDirX, &gravityDirY);
	}
	/*	if ( fabs(accelX) > fabs(accelY) )
	{
	if ( accelX > 0 )
	{
	gravityDirX = 1;
	gravityDirY = 0;
	}
	else
	{
	gravityDirX = -1;
	gravityDirY = 0;
	}
	}
	else
	{
	if ( accelY > 0 )
	{
	gravityDirX = 0;
	gravityDirY = -1;
	}
	else
	{
	gravityDirX = 0;
	gravityDirY = 1;
	}
	}
	* /
	if (gravityDirX != oldGravityDirX || gravityDirY != oldGravityDirY)
	{
		//	float snowSpeed = 2;
		//	[snowU setGlobalVelX:gravityDirX*snowSpeed Y:gravityDirY*snowSpeed];
		//	[snowD setGlobalVelX:gravityDirX*snowSpeed Y:gravityDirY*snowSpeed];
		//	[snowL setGlobalVelX:gravityDirX*snowSpeed Y:gravityDirY*snowSpeed];
		//	[snowR setGlobalVelX:gravityDirX*snowSpeed Y:gravityDirY*snowSpeed];

		//		[water setGravityDirX:gravityDirX Y:gravityDirY];
	}
}
*/

void tickPlay()
{
	for (int Idx = 0; Idx < numMovables; Idx++)
	{
		movables[Idx].shape.numFramesSinceLastCollision++;
	}

	if (touchingObject)
	{
		if (touchingObject->numFramesSinceLastCollision < freezeWhenHitDuration)
		{
			//touchingObject = NULL;
		}
		else
		{
			moveObjectBy(touchingObject, touchingPosX - handledTouchUpToX, touchingPosY - handledTouchUpToY);
			handledTouchUpToX = touchingPosX;
			handledTouchUpToY = touchingPosY;
		}
	}

	for (int Idx = 0; Idx < numInGame; Idx++)
	{
		if (inGame[Idx]->shapetype == SHAPE_rigidbody)
			rbtick((RigidBody*)inGame[Idx]);
	}
}

void WalkerGame::drawView(SpriteBatch* spriteBatch)
{
	// Replace the implementation of this method to do your own custom drawing

	/*    const GLfloat squareVertices[] = {
	-0.5f, -0.5f,
	0.5f,  -0.5f,
	-0.5f,  0.5f,
	0.5f,   0.5f,
	};
	const GLubyte squareColors[] = {
	255, 255,   0, 255,
	0,   255, 255, 255,
	0,     0,   0,   0,
	255,   0, 255, 255,
	};*/

	//C	[EAGLContext setCurrentContext : context];

	//C	glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
	//C	glViewport(0, 0, backingWidth, backingHeight);

	//C	glEnableClientState(GL_VERTEX_ARRAY);
	//C	glDisableClientState(GL_COLOR_ARRAY);

	/*    glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(-1.0f, 1.0f, -1.5f, 1.5f, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glRotatef(3.0f, 0.0f, 0.0f, 1.0f);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glVertexPointer(2, GL_FLOAT, 0, squareVertices);
	glEnableClientState(GL_VERTEX_ARRAY);
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, squareColors);
	glEnableClientState(GL_COLOR_ARRAY);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	*/
//C	if (playing)
//C	{
//C		tickPlay();
//C	}
//C	else
//C	{
//C		tickEditor();
//C	}

	//C glMatrixMode(GL_PROJECTION);
	//C	glLoadIdentity();
	//C	glOrthof(0, 320, 480, 0, -1, 1);

	//C	glMatrixMode(GL_MODELVIEW);
	//C	glLoadIdentity();

	//	if ( touchingMovable < 0 )
	//	{
	//		cameraPosX = blob.x-160;
	//		cameraPosY = blob.y-240;
	//	}

	if (showingObjects)
	{
		//C	glClearColor(0.0, 0.0, 0.0, 1.0f);
		//C		glClear(GL_COLOR_BUFFER_BIT);

		for (int Idx = 0; Idx < numTemplates; Idx++)
		{
			drawObject(templates[Idx], spriteBatch);
		}
	}
	else
	{
		//C		glClearColor(0.25, 0.25, 0.25, 1.0f);
		//C		glClear(GL_COLOR_BUFFER_BIT);

		//C		glTranslatef(-cameraPosX, -cameraPosY, 0);

		//C		daySetSingleTexturing(packedTexture);
		//C		daySetTransparent();

		for (int Idx = 0; Idx < numInGame; Idx++)
		{
			drawObject(inGame[Idx], spriteBatch);
		}
	}

	//	daySetFlatColor();
	//	[water draw];

/*C	if (showingEditor)
	{
		daySetSingleTexturing(packedTexture);

		float toolsX = 35, toolsY = 30;
		float toolSpacing = 58;
		glColor4f(0.5, 0.5, 1, 1);
		daySetTile(&toolButtonTile);
		dayDrawQuadCentered(toolsX, toolsY);
		if (playing)
		{
			glColor4f(0.5, 1, 0.5, 1);
			daySetTile(&playToolTile);
			dayDrawQuadCentered(toolsX, toolsY);
		}
		else
		{
			GameShape* cloudObject = touchingObject;
			if (!cloudObject)
				cloudObject = lastTouchedObject;

			// resize handles
			if (showingResize && cloudObject && canResize(cloudObject))
			{
				BoundingBox* resizableBox = &((MovableSegment*)cloudObject)->touchableShape;
				float offset = 30;

				glColor4f(1, 1, 1, 1);
				daySetTile(&resizeHandleTile);
				dayDrawQuadCenteredScaled(resizableBox->minx - offset, resizableBox->miny - offset, 1, 1);
				dayDrawQuadCenteredScaled(resizableBox->maxx + offset, resizableBox->miny - offset, -1, 1);
				dayDrawQuadCenteredScaled(resizableBox->maxx + offset, resizableBox->maxy + offset, -1, -1);
				dayDrawQuadCenteredScaled(resizableBox->minx - offset, resizableBox->maxy + offset, 1, -1);
			}

			// button 1 - play
			glColor4f(0.9, 0.4, 0.4, 1);
			daySetTile(&stopToolTile);
			dayDrawQuadCentered(toolsX, toolsY);

			// button 2 - ?
			glColor4f(0.5, 0.5, 1, 1);
			daySetTile(&toolButtonTile);
			dayDrawQuadCentered(toolSpacing + toolsX, toolsY);

			if (cloudObject && !cloudObject->erased)
			{
				// button 3 - resize
				if (canResize(cloudObject))
				{
					// show the resize tool
					glColor4f(0.5, 0.5, 1, 1);
					daySetTile(&toolButtonTile);
					dayDrawQuadCentered(toolSpacing * 2 + toolsX, toolsY);

					drawObjectAt(cloudObject, toolSpacing * 2 + toolsX, toolsY, 8);

					if (showingResize)
						glColor4f(1, 1, 1, 1);
					else
						glColor4f(0, 0, 0, 1);
					daySetTile(&resizeToolTile);
					dayDrawQuadCentered(toolSpacing * 2 + toolsX, toolsY);
				}

				// button 4 - delete
				glColor4f(0.5, 0.5, 1, 1);
				daySetTile(&toolButtonTile);
				dayDrawQuadCentered(toolSpacing * 3 + toolsX, toolsY);

				drawObjectAt(cloudObject, toolSpacing * 3 + toolsX, toolsY, 20);

				glColor4f(1, 0, 0, 1);
				daySetTile(&cancelToolTile);
				dayDrawQuadCentered(toolSpacing * 3 + toolsX, toolsY);
			}

			//			glColor4f(0.8,0.8,0.4,1);
			//			daySetTile(&selectToolTile);
			//			dayDrawQuadCentered(toolSpacing+toolsX,toolsY);

			//			glColor4f(1,0,0,1);
			//			daySetTile(&cancelToolTile);
			//			dayDrawQuadCentered(toolSpacing+toolsX+5,toolsY);

			//			if ( showingObjects )
			//				glColor4f(0.9,0.9,0.3,1);
			//			else
			//				glColor4f(0.7,0.7,0.7,1);
			//			daySetTile(&objectsToolTile);
			//			dayDrawQuadCentered(toolSpacing*2+toolsX,toolsY);

			//			glColor4f(0.6,0.4,0.9,1);
			//			daySetTile(&undoToolTile);
			//			dayDrawQuadCentered(toolSpacing*3+toolsX,toolsY);

			glColor4f(1, 1, 1, 1);
			daySetTile(&thoughtBubbleTile);
			dayDrawQuadCentered(cloudPosX, cloudPosY);

			if (cloudObject)
			{
				drawObjectAt(cloudObject, cloudPosX, cloudPosY, 25);
			}
		}
	} C*/

//C	glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
//C	[context presentRenderbuffer : GL_RENDERBUFFER_OES];
}
