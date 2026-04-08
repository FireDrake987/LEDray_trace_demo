#pragma once
#include "Plane.h"
#include "Point3D.h"
#include "Material.h"

class Triangle : public Plane {
	Point3D p1;
	Point3D p2;
	Point3D p3;

public: 
	//
	//  FUNCTION: Triangle(Material mat, Point3D p1, Point3D p2, Point3D p3)
	//
	//  PURPOSE: Constructor for Triangle class. Takes in a material and three points that define the triangle. The plane of the triangle is calculated from the three points, and the material is passed to the Plane constructor.
	//
	Triangle(Material mat, Point3D p1, Point3D p2, Point3D p3);

	//
	//  FUNCTION: Triangle()
	//
	//  PURPOSE: Default constructor for Triangle class. Initializes the three points to (0, 0, 0) and the plane to the default plane (A=0, B=0, C=0, D=0).
	//
	Triangle();


	//
	//  FUNCTION: getIntersection(Ray ray)
	//
	//  PURPOSE: Overrides the getIntersection function from the Plane class. First, it calls the Plane's getIntersection to find the intersection point with the plane of the triangle. If there is no intersection (t < 0), it returns that result. If there is an intersection, it checks if the intersection point is inside the triangle using barycentric coordinates. If it is not inside, it sets t to -1 to indicate no valid intersection.
	//
	virtual intersectionInfoStruct getIntersection(Ray ray);

	//
	//   FUNCTION: triangleArea(Point3D p1, Point3D p2, Point3D p3)
	//
	//   PURPOSE: Static function that calculates the area of a triangle defined by three points in 3D space. It uses the cross product of two vectors formed by the points to find the area.
	//
	static double triangleArea(Point3D p1, Point3D p2, Point3D p3);
};
