#pragma once
#include <ostream>
class Point3D {
private:
	double x, y, z;

public:
	Point3D(double x, double y, double z);

	//Operators
	inline Point3D operator+(const Point3D& other) const {
		return Point3D(this->getX() + other.getX(), this->getY() + other.getY(), this->getZ() + other.getZ());
	}
	inline Point3D operator+=(const Point3D& other) {
		this->x = this->getX() + other.getX();
		this->y = this->getY() + other.getY();
		this->z = this->getZ() + other.getZ();
		return Point3D(this->x, this->y, this->z);
	}
	inline Point3D operator-(const Point3D& other) const {
		return Point3D(this->getX() - other.getX(), this->getY() - other.getY(), this->getZ() - other.getZ());
	}
	inline Point3D operator-=(const Point3D& other) {
		this->x = this->getX() - other.getX();
		this->y = this->getY() - other.getY();
		this->z = this->getZ() - other.getZ();
		return Point3D(this->x, this->y, this->z);
	}
	inline Point3D operator*(const int& other) const {
		return Point3D(this->getX() * other, this->getY() * other, this->getZ() * other);
	}
	inline Point3D operator*=(const int& other) {
		this->x = this->getX() * other;
		this->y = this->getY() * other;
		this->z = this->getZ() * other;
		return Point3D(this->x, this->y, this->z);
	}
	inline friend Point3D operator*(const int& scalar, const Point3D& point) {
		return Point3D(point.getX() * scalar, point.getY() * scalar, point.getZ() * scalar);
	}
	inline friend Point3D operator*=(const int& other, Point3D& point) {
		point.x = point.getX() * other;
		point.y = point.getY() * other;
		point.z = point.getZ() * other;
		return Point3D(point.x, point.y, point.z);
	}
	inline Point3D operator/(const int& other) const {
		return Point3D(this->getX() / other, this->getY() / other, this->getZ() / other);
	}
	inline friend std::ostream& operator<<(std::ostream& os, const Point3D& point) {
		os << "(" << point.x << ", " << point.y << ", " << point.z << ")";
		return os;
	}
	inline bool operator==(const Point3D& other) const {
		return this->getX() == other.getX() && this->getY() == other.getY() && this->getZ() == other.getZ();
	}
	inline bool operator!=(const Point3D& other) const {
		return !(this == &other);
	}

	//Get Methods
	inline double getX() const {
		return this->x;
	};
	inline double getY() const {
		return this->y;
	};
	inline double getZ() const {
		return this->z;
	};
};