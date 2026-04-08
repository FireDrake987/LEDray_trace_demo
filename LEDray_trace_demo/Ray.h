#pragma once
#include "Point3D.h"
#include "Vector.h"

class Ray {
	Point3D start;
	Vector dir;

public: 
	//
	//  FUNCTION: Ray(Point3D origin, Vector direction)
	//
	//  PURPOSE: Constructor for Ray class. Takes in an origin point and a direction vector. Additionally normalized direction vector to ensure consistent behavior in intersection calculations.
	//
	Ray(Point3D origin, Vector direction);

	//
	//  FUNCTION: Ray(Vector direction)
	//
	//  PURPOSE: Constructor for Ray class that takes in only a direction vector. The origin point is set to (0, 0, 0) by default. The direction vector is normalized to ensure consistent behavior in intersection calculations.
	//
	Ray(Vector asVector);

	//Get Methods
	inline Point3D getStart() const { return start; }

	inline Point3D getEnd() const { return start + dir.asPoint(); }

	inline Vector getVector() const { return dir; }
};
