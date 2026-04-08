#pragma once
#include "intersectionInfoStruct.h"
#include "Ray.h"
#include "Point3D.h"
#include "Material.h"

class Plane {
	double A, B, C, D;//Ax+By+Cz+D=0
	Point3D pointOnPlane;
	Material mat;

public:
	//
	//  FUNCTION: Plane(Material mat, Point3D p1, Point3D p2, Point3D p3)
	//
	//  PURPOSE: Constructor for Plane class. Takes in a material and three points that define the plane. The plane is calculated from the three points, and the material is stored for later use in shading calculations.
	//
	Plane(Material mat, Point3D p1, Point3D p2, Point3D p3);

	//
	//  FUNCTION: Plane(Material mat, double a, double b, double c, double d)
	//
	//  PURPOSE: Constructor for Plane class. Takes in a material and the coefficients of the plane equation (A, B, C, D) in the form Ax + By + Cz + D = 0. The material is stored for later use in shading calculations, and a point on the plane is calculated based on the coefficients for use in intersection calculations.
	//
	//  COMMENTS: 
	//
	//       Some implementations use Ax+By+Cz=D, but this uses Ax+By+Cz+D=0
	//       If you expect Ax+By+Cz+D=0, just set D to -D when calling this constructor
	//
	Plane(Material mat, double a, double b, double c, double d);

	//
	//  FUNCTION: Plane()
	// 
	//  PURPOSE: Default constructor for Plane class. Initializes the plane to the default plane (A=0, B=0, C=0, D=0) and the material to a default material.
	//
	Plane();


	//
	//  FUNCTION: getIntersection(Ray ray)
	//
	//  PURPOSE: Calculates the intersection of the given ray with the plane. It uses the plane equation and the ray equation to solve for the parameter t at which the ray intersects the plane. If there is no intersection (ray is parallel to the plane), it returns an intersectionInfoStruct with t set to -1. If there is an intersection, it calculates the intersection point and returns it along with the parameter t.
	//
	//  COMMENTS: 
	//
	//         Override this method in subclasses to implement different intersection behavior (e.g., for triangles, only return an intersection if the point is within the triangle bounds)
	//
	virtual intersectionInfoStruct getIntersection(Ray ray);


	//Get Methods
	inline double getA() const { return A; }

	inline double getB() const { return B; }

	inline double getC() const { return C; }

	inline double getD() const { return D; }

	inline Material getMaterial() const { return mat; }

	inline Point3D getPointOnPlane() const { return pointOnPlane; }

	Vector getNormal() const;
};
