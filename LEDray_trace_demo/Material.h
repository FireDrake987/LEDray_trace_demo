#pragma once
#include "BGRPixel.h"
#include "Point3D.h"
#include <vector>
#include "Ray.h"

class Plane;//Forward declaration
class Camera;//Forward declaration

class Material {
	BGRPixel col;

public: 
	Material(BGRPixel col);
	Material();

	virtual BGRPixel getColAtPoint(Point3D intPoint, Camera *cam, Ray &ray, Plane *plane, int str) const { return col; }
};
