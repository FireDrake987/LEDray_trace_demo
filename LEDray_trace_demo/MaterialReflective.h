#pragma once
#include "Material.h"

//TODO: Implement this class. It should represent a reflective material, which reflects some light that hits it. The getColAtPoint function should calculate the reflection ray based on the incoming ray and the normal of the plane, and then trace that ray to find the color of the reflected ray. The reflectance variable can be used to control how much of the reflected color is blended with the base color of the material (e.g., if reflectance is 0.5, then the final color would be an equal blend of the reflected color and the base color).
class MaterialReflective : public Material {
	double reflectance;
public:
	MaterialReflective(double reflectance, BGRPixel col);
	MaterialReflective(BGRPixel col);
	MaterialReflective();
	virtual BGRPixel getColAtPoint(Point3D intPoint, Camera* cam, Ray &ray, Plane *plane, int str) const override;
};