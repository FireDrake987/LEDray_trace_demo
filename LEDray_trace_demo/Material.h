#pragma once
#include "BGRPixel.h"
#include "Point3D.h"

class Material {
	BGRPixel col;
	double luminance;//TODO: add
	double reflectance;//TODO: add
	double specularity;//TODO: add (0(diffuse)-1(specular))
	double absorbance;//TODO: add (0(no light absorbed)-1(all light absorbed))
	double transmittance;//TODO: add
	double refractiveIndex;//TODO: add

public: 
	Material(BGRPixel col, double luminance, double reflectance, double specularity, double absorbance, double trasmittance, double refractiveIndex);
	Material(BGRPixel col);
	Material();

	BGRPixel getColAtPoint(Point3D relPoint, Point3D absPoint) const { return col; };
	BGRPixel getColAtPoint(Point3D relPoint) const { return col; }
};
