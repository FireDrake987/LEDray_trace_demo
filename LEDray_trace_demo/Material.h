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
	//
	//  FUNCTION: Material(BGRPixel col)
	//
	//  PURPOSE: Constructor for Material class. Takes in a BGRPixel that represents the base color of the material. This color will be returned by the getColAtPoint function for any point on the material, regardless of lighting or other factors.
	//
	Material(BGRPixel col);

	//
	//  FUNCTION: Material()
	//
	//  PURPOSE: Default constructor for Material class. Initializes the base color to black (B=0, G=0, R=0).
	//
	Material();


	//
	//  FUNCTION: getColAtPoint(Point3D intPoint, Camera* cam, Ray &ray, Plane *plane, int str)
	//
	//  PURPOSE: Returns the color of the material at the given intersection point. For the base Material class, this simply returns the base color of the material, ignoring lighting, reflections, and other factors. This function can be overridden in subclasses to implement more complex shading models (e.g., reflective materials, etc.) that take into account lighting and other factors to calculate the final color at the intersection point.
	//
	virtual BGRPixel getColAtPoint(Point3D intPoint, Camera *cam, Ray &ray, Plane *plane, int str) const { return col; }
};
