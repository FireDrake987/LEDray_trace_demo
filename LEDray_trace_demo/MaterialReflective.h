#pragma once
#include "Material.h"

class MaterialReflective : public Material {
	double reflectance;
public:
	MaterialReflective(double reflectance, BGRPixel col);
	MaterialReflective(BGRPixel col);
	MaterialReflective();
	virtual BGRPixel getColAtPoint(Point3D intPoint, Camera* cam, Ray &ray, Plane *plane, int str) const override;
};