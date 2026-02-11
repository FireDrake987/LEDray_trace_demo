#pragma once
#include "Quaternion.h"
#include "BGRPixel.h"
#include "Plane.h"
#include "Material.h"
#include <vector>

class Camera {
	inline static double RENDER = DBL_MAX;
	inline static BGRPixel DEFAULT_COLOR = BGRPixel{0, 0, 0};
	double x, y, z;
	double width, height;
	double FOVx, FOVy;
	Quaternion camRot;
	std::vector<std::vector<Vector>> map;
	std::vector<std::vector<BGRPixel>> output;
	bool ready;

public:
	std::vector<Plane> scene;

	enum CameraType {
		FLAT, CURVED
	};
	inline static CameraType type = FLAT;

	Camera(double x, double y, double z, double width, double height, Quaternion camRot);
	Camera();

	inline void rawMove(double dx, double dy, double dz) {
		x += dx;
		y += dy;
		z += dz;
	}
	void move(Vector dir);//Respecting camRot TODO: add
	void move(double right, double down, double forwards);//Same thing as vector form TODO: add
	inline void setRot(Quaternion newRot) {
		camRot = newRot;
		invalidate();
	}
	inline void rotate(Quaternion rot) {
		camRot = camRot * rot;
		invalidate();
	}
	void invalidate();//Sets ready=false, clears output and map
	void build();//Rebuild Camera based on current data, sets ready=true
	void buildMap();//Initializes map to proper mapping based on current configurations, called by regenerate

	std::vector<std::vector<BGRPixel>> render(int x1, int y1, int x2, int y2);//Automatically calls build() when ready!=true

	Vector angleToVector(double yaw, double pitch);
};
