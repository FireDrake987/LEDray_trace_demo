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

	inline static double minStr = 0.05;//Minimum strength for a ray to be rendered; this is used to prevent rendering rays that are too weak to contribute to the final image, which can improve performance by skipping unnecessary calculations for rays that would have negligible impact on the final color of the pixel. The value can be adjusted based on the desired balance between performance and visual quality (e.g., a higher value will skip more rays and improve performance but may result in a less accurate rendering, while a lower value will include more rays and improve visual quality but may reduce performance).

	//
	//  FUNCTION: Camera(double, double, double, double, double, Quaternion)
	//
	//  PURPOSE: Constructor for Camera class. Initializes the camera's position (x, y, z), viewport dimensions (width, height), and orientation (camRot). The field of view is set to default values (FOVx = 90 degrees, FOVy = 75 degrees), and the camera is marked as not ready until the build function is called to generate the ray mapping.
	//
	Camera(double x, double y, double z, double width, double height, Quaternion camRot);

	//
	//  FUNCTION: Camera()
	//
	//  PURPOSE: Default constructor for Camera class. Initializes the camera's position to the origin (0, 0, 0), viewport dimensions to 0, orientation to the identity rotation (no rotation), and field of view to default values (FOVx = 90 degrees, FOVy = 75 degrees). The camera is marked as not ready until the build function is called to generate the ray mapping.
	//
	//  COMMENTS:
	//
	//       This camera will have no output. This is effectively invalid, but can be used as an effective placeholder until the actual camera is initialized with the other constructor, which can be useful for certain data structures that require a default constructor (e.g., if you want to have a vector of cameras, you need a default constructor to initialize the vector).
	//
	Camera();


	//
	//  FUNCTION: rawMove(double, double, double)
	//
	//  PURPOSE: Moves the camera by the given raw amounts in the x/y/z directions. This is a simple translation that does not take into account the camera's current orientation; it simply adds the given amounts to the camera's current position. The invalidate function is called to mark the camera as not ready and clear the ray mapping, since the camera's position has changed and the existing ray mapping may no longer be valid.
	//
	inline void rawMove(double dx, double dy, double dz) {
		std::unique_lock<std::shared_mutex> lock(invalidateMut);
		x += dx;
		y += dy;
		z += dz;
		invalidate();
	}

	//
	//  FUNCTION: move(Vector)
	//
	//  PURPOSE: Moves the camera by the given vector, which represents the direction and magnitude of the movement. The vector is transformed by the camera's current rotation to ensure that the movement is relative to the camera's orientation (e.g., moving forward will move in the direction the camera is facing). The invalidate function is called to mark the camera as not ready and clear the ray mapping, since the camera's position has changed and the existing ray mapping may no longer be valid.
	//
	void move(Vector dir);

	//
	//  FUNCTION: move(double, double, double)
	//
	//  PURPOSE: Moves the camera by the given amounts in the right, down, and forward directions relative to the camera's current orientation. The right direction is determined by applying the camera's rotation to the vector (1, 0, 0), the down direction is determined by applying the camera's rotation to the vector (0, 1, 0), and the forward direction is determined by applying the camera's rotation to the vector (0, 0, 1). The given amounts are multiplied by these direction vectors and added to the camera's current position to move it accordingly. The invalidate function is called to mark the camera as not ready and clear the ray mapping, since the camera's position has changed and the existing ray mapping may no longer be valid.
	//
	void move(double right, double down, double forwards);

	//
	//  FUNCTION: setRot(Quaternion)
	//
	//  PURPOSE: Sets the camera's rotation to the given quaternion, which represents the new orientation of the camera. The invalidate function is called to mark the camera as not ready and clear the ray mapping, since the camera's orientation has changed and the existing ray mapping may no longer be valid.
	//
	inline void setRot(Quaternion newRot) {
		std::unique_lock<std::shared_mutex> lock(invalidateMut);
		camRot = newRot;
		invalidate();
	}

	//
	//  FUNCTION: rotate(Quaternion)
	//
	//  PURPOSE: Rotates the camera by the given quaternion, which represents the rotation to be applied to the camera's current orientation. The new orientation is calculated by multiplying the current rotation (camRot) by the given rotation (rot), which applies the new rotation on top of the existing orientation. The invalidate function is called to mark the camera as not ready and clear the ray mapping, since the camera's orientation has changed and the existing ray mapping may no longer be valid.
	//
	inline void rotate(Quaternion rot) {
		std::unique_lock<std::shared_mutex> lock(invalidateMut);
		camRot = camRot * rot;
		invalidate();
	}

	//
	//  FUNCTION: eulerRotate(double, double)
	//
	//  PURPOSE: Rotates the camera by the given yaw and pitch angles, which represent rotations around the vertical (y) axis and horizontal (x) axis, respectively. The roll angle is not modified in this function. The yaw rotation is applied first, followed by the pitch rotation. The invalidate function is called to mark the camera as not ready and clear the ray mapping, since the camera's orientation has changed and the existing ray mapping may no longer be valid.
	//
	//  COMMENTS:
	//
	//       This function is designed for implementing FPS-style camera controls, where the yaw rotation is applied around the global up axis (y-axis) to prevent roll drift, and the pitch rotation is applied around the camera's local right axis to allow for looking up and down without affecting the yaw rotation.
	//
	void eulerRotate(double yaw, double pitch);

	//
	//  FUNCTION: eulerRotate(double, double, double)
	//
	//  PURPOSE: Rotates the camera by the given yaw, pitch, and roll angles, which represent rotations around the vertical (y) axis, horizontal (x) axis, and forward (z) axis, respectively. The yaw rotation is applied first, followed by the pitch rotation, and then the roll rotation. The invalidate function is called to mark the camera as not ready and clear the ray mapping, since the camera's orientation has changed and the existing ray mapping may no longer be valid.
	//
	//  COMMENTS:
	//
	//       This function allows for full 3D rotation of the camera, including roll, which can be useful for certain applications (e.g., flight simulators). However, it may not be suitable for FPS-style camera controls due to potential roll drift when applying yaw rotations around the camera's local up axis.
	//
	void eulerRotate(double yaw, double pitch, double roll);

	//
	//  FUNCTION: invalidate()
	//
	//  PURPOSE: Marks the camera as not ready and clears the ray mapping. This function is called whenever the camera's position or orientation changes, since those changes may invalidate the existing ray mapping. The ready flag is set to false to indicate that the camera needs to be rebuilt before it can be used for rendering, and the map is cleared to free up memory and ensure that a new mapping will be generated when build() is called.
	//
	//  COMMENTS:
	//
	//       This function is thread-safe
	//
	void invalidate();

	//
	//  FUNCTION: build()
	//
	//  PURPOSE: Rebuilds the camera's ray mapping based on the current position, orientation, and field of view. This function generates the map of direction vectors for each pixel in the viewport, which will be used for tracing rays during rendering. The ready flag is set to true after the mapping is built to indicate that the camera is ready for rendering.
	//
	//  COMMENTS:
	//
	//       This function is thread-safe and will automatically check if the camera is already ready before rebuilding, so it can be safely called from the render function without needing to worry about redundant builds.
	//       It is unnecessary to ever call this function directly, as it will be automatically called by the render function when needed. However, it can be called directly if you want to pre-build the camera before rendering to avoid the overhead of building during the first render call.
	//       This function will do nothing if invalidate() has not been called since the last build, so it is safe to call multiple times without needing to worry about redundant builds.
	//
	void build();//Rebuild Camera based on current data, sets ready=true

	//
	//  FUNCTION: buildMap()
	//
	//  PURPOSE: Generates the map of direction vectors for each pixel in the viewport based on the camera's current orientation and field of view. This function calculates the direction vector for each pixel by applying the camera's rotation to the appropriate base direction vector for that pixel, which is determined by the field of view and the pixel's position in the viewport. The resulting direction vectors are stored in the map, which will be used for tracing rays during rendering.
	//
	//  COMMENTS:
	//
	//       This function is NOT thread-safe and should only be called from within the build() function, which handles the necessary locking to ensure thread safety. It is not intended to be called directly from outside the Camera class, as it does not handle the ready flag or any necessary locking.
	//       If you call this function directly, you must ensure that you have acquired the appropriate lock on invalidateMut
	//
	void buildMap();

	//
	//  FUNCTION: render(int, int, int, int)
	//
	//  PURPOSE: Renders the scene from the camera's perspective for the specified rectangular region of the viewport defined by the top-left corner (x1, y1) and the bottom-right corner (x2, y2). This function traces rays for each pixel in the specified region using the precomputed direction vectors from the map and returns a vector of BGRPixel colors corresponding to the rendered output. If the camera is not ready (i.e., if invalidate() has been called since the last build), this function will automatically call build() to regenerate the ray mapping before rendering.
	//
	//  COMMENTS:
	//
	//       Automatically calls build() when ready!=true, so you can safely call this function without needing to worry about whether the camera is ready or not. However, if you want to pre-build the camera before rendering to avoid the overhead of building during the first render call, you can call build() directly before calling render().
	//
	std::vector<BGRPixel> render(int x1, int y1, int x2, int y2);

	//
	//  FUNCTION: traceRay(Ray, double)
	//
	//  PURPOSE: Traces the given ray through the scene and returns the color of the pixel that the ray intersects. The str parameter represents the strength of the ray, which can be used to control how much the color is affected by reflections and other factors (e.g., if str is less than a certain threshold, it might return a default color instead of tracing further to avoid infinite recursion). This function iterates through all objects in the scene to find the closest intersection with the ray, and then calculates the color at that intersection point based on the material properties of the object and any lighting or reflections.
	//
	BGRPixel traceRay(Ray ray, double str);

	//
	//  FUNCTION: angleToVector(double, double)
	//
	//  PURPOSE: Converts the given yaw and pitch angles (in radians) to a direction vector in 3D space. The yaw angle represents rotation around the vertical (y) axis, and the pitch angle represents rotation around the horizontal (x) axis. The resulting vector is calculated by applying the appropriate trigonometric functions to convert the angles into a direction vector that can be used for ray tracing.
	//
	Vector angleToVector(double yaw, double pitch);

	//
	//  FUNCTION: setFOV(double, double)
	//
	//  PURPOSE: Sets the camera's field of view in the x and y directions (FOVx and FOVy) to the given values (in radians). The invalidate function is called to mark the camera as not ready and clear the ray mapping, since the field of view has changed and the existing ray mapping may no longer be valid.
	//
	void setFOV(double x, double y) {
		std::unique_lock<std::shared_mutex> lock(invalidateMut);
		this->FOVx = x;
		this->FOVy = y;
		invalidate();
	}
};
