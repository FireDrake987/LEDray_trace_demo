#pragma once
#include "BGRPixel.h"
#include "Point3D.h"
#include <vector>

class Plane;//Forward declaration

class Material {
	BGRPixel col;

public: 
	Material(BGRPixel col);
	Material();

	virtual BGRPixel getColAtPoint(Point3D intPoint, std::vector<Plane*> &scene) const { return col; }
};
