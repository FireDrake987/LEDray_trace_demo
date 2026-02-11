#pragma once
#include "Point3D.h"
#include "Vector.h"

class Ray {
	Point3D start;
	Vector dir;

public: 
	Ray(Point3D origin, Vector direction);
	Ray(Vector asVector);

	inline Point3D getStart() const { return start; }
	inline Point3D getEnd() const { return start + dir.asPoint(); }
	inline Vector getVector() const { return dir; }
};
