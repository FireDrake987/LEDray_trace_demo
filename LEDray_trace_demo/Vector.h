#pragma once
#include "Point3D.h"
class Vector {
private: 
	double x, y, z;

public: 
	Vector(double Vx, double Vy, double Vz);
	Vector(Point3D start, Point3D end);
	Vector(Point3D fromPoint);

	//Methods
	inline Vector cross(const Vector& other) const {
		return Vector(this->getY() * other.getZ() - this->getZ() * other.getY(), this->getX() * other.getZ() - this->getZ() * other.getX(), this->getX() * other.getY() - this->getY() * other.getX());
	}

	inline static Vector cross(const Vector& vec1, const Vector& vec2) {
		return vec1.cross(vec2);
	}

	inline double dot(const Vector& other) const {
		return this->getX() * other.getX() + this->getY() * other.getY() + this->getZ() * other.getZ();
	}

	inline static double dot(const Vector& vec1, const Vector& vec2) {
		return vec1.dot(vec2);
	}

	inline Vector normalize() const {
		double sum = getX() + getY() + getZ();
		return Vector(getX()/sum, getY()/sum, getZ()/sum);
	}

	//Operators
	inline Vector operator+(const Vector& other) const {
		return Vector(asPoint() + other.asPoint());
	}
	inline Vector operator+=(const Vector& other) {
		this->x = this->getX() + other.x;
		this->y = this->getY() + other.y;
		this->z = this->getZ() + other.z;
		return Vector(this->x, this->y, this->z);
	}
	inline Vector operator-(const Vector& other) const {
		return Vector(asPoint() - other.asPoint());
	}
	inline Vector operator-=(const Vector& other) {
		this->x = this->getX() - other.x;
		this->y = this->getY() - other.y;
		this->z = this->getZ() - other.z;
		return Vector(this->x, this->y, this->z);
	}
	inline Vector operator*(const int& scalar) const {
		return Vector(asPoint() * scalar);
	}
	inline Vector operator*=(const int& other) {
		this->x = this->getX() * other;
		this->y = this->getY() * other;
		this->z = this->getZ() * other;
		return Vector(this->x, this->y, this->z);
	}
	inline friend Vector operator*(const int& scalar, const Vector& vector) {
		return Vector(vector.asPoint() * scalar);
	}
	inline Vector operator/(const int& scalar) const {
		return Vector(asPoint() / scalar);
	}
	inline Vector operator/=(const int& scalar) {
		this->x = this->getX() / scalar;
		this->y = this->getY() / scalar;
		this->z = this->getZ() / scalar;
	}
	inline friend std::ostream& operator<<(std::ostream& os, const Vector& vec) {
		os << vec.getX() << "i" << (vec.getY() < 0 ? "" : "+") << vec.getY() << "j" << (vec.getZ() < 0 ? "" : "+") << vec.getZ() << "k";
		return os;
	}
	inline bool operator==(const Vector& other) {
		return (getX() == other.getX() && getY() == other.getY() && getZ() == other.getZ());
	}
	inline bool operator!=(const Vector & other) {
		return !(*this == other);
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
	inline Point3D asPoint() const {
		return Point3D(getX(), getY(), getZ());
	};
};