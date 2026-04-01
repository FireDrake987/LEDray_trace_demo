#pragma once
#include "Quaternion.h"
#include "BGRPixel.h"
#include "Plane.h"
#include "Material.h"
#include <vector>
#include <mutex>
#include <shared_mutex>

class Camera {
	inline static double RENDER = DBL_MAX;
	inline static BGRPixel DEFAULT_COLOR = BGRPixel{0, 0, 0};
	double x, y, z;
	double width, height;
	double FOVx, FOVy;
	Quaternion camRot;
	std::vector<std::vector<Vector>> map;
	bool ready;

public:
	std::vector<Plane*> scene;
	std::shared_mutex invalidateMut;

	enum CameraType {
		FLAT, CURVED
	};
	inline static CameraType type = CURVED;

	Camera(double x, double y, double z, double width, double height, Quaternion camRot);
	Camera();

	inline void rawMove(double dx, double dy, double dz) {
		std::unique_lock<std::shared_mutex> lock(invalidateMut);
		x += dx;
		y += dy;
		z += dz;
		invalidate();
	}
	void move(Vector dir);
	void move(double right, double down, double forwards);
	inline void setRot(Quaternion newRot) {
		std::unique_lock<std::shared_mutex> lock(invalidateMut);
		camRot = newRot;
		invalidate();
	}
	inline void rotate(Quaternion rot) {
		std::unique_lock<std::shared_mutex> lock(invalidateMut);
		camRot = camRot * rot;
		invalidate();
	}
	void eulerRotate(double yaw, double pitch);
	void eulerRotate(double yaw, double pitch, double roll);

	void invalidate();//Sets ready=false, clears output and map
	void build();//Rebuild Camera based on current data, sets ready=true
	void buildMap();//Initializes map to proper mapping based on current configurations, called by regenerate

	std::vector<BGRPixel> render(int x1, int y1, int x2, int y2);//Automatically calls build() when ready!=true

	BGRPixel traceRay(Ray ray, double str);

	Vector angleToVector(double yaw, double pitch);

	void setFOV(double x, double y) {
		std::unique_lock<std::shared_mutex> lock(invalidateMut);
		this->FOVx = x;
		this->FOVy = y;
		invalidate();
	}
};
