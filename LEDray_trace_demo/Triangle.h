#pragma once
#include "Plane.h"
#include "Point3D.h"
#include "Material.h"

class Triangle : public Plane {
	Point3D p1;
	Point3D p2;
	Point3D p3;

public: 
	Triangle(Material mat, Point3D p1, Point3D p2, Point3D p3);
	Triangle();

	virtual intersectionInfoStruct getIntersection(Ray ray);

	static double triangleArea(Point3D p1, Point3D p2, Point3D p3);
};
