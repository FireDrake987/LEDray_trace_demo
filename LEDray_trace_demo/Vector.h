#pragma once
#include "Point3D.h"
class Vector {
private: 
	double x, y, z;

public: 
	//
	//  FUNCTION: Vector::Vector(double, double, double)
	//
	//  PURPOSE: Initializes a vector with the given raw x/y/z components
	//
	Vector(double Vx, double Vy, double Vz);

	//
	//  FUNCTION: Vector::Vector(Point3D, Point3D)
	//
	//  PURPOSE: Initializes a vector as the difference between the two given points, representing the direction and magnitude from the first point to the second point
	//
	//  COMMENTS:
	//
	//         This will be equivalent to end minus start
	//
	Vector(Point3D start, Point3D end);

	//
	//  FUNCTION: Vector::Vector(Point3D)
	//
	//  PURPOSE: Initializes a vector with the same components as the given point, treating the point as a vector from the origin to that point
	//
	Vector(Point3D fromPoint);

	//
	//  FUNCTION: Vector::Vector()
	//
	//  PURPOSE: Initializes a vector with x/y/z components all set to 0, representing the zero vector
	//
	Vector();


	//
	//  FUNCTION: Vector::cross(Vector)
	//
	//  PURPOSE: Returns the cross product of this vector with the given other vector, which is a vector that is perpendicular to both original vectors and has a magnitude equal to the area of the parallelogram formed by the two vectors
	//
	inline Vector cross(const Vector& other) const {
		return Vector(this->getY() * other.getZ() - this->getZ() * other.getY(), this->getZ() * other.getX() - this->getX() * other.getZ(), this->getX() * other.getY() - this->getY() * other.getX());
	}

	//
	//  FUNCTION: Vector::cross(Vector, Vector)
	//
	//  PURPOSE: Returns the cross product of the two given vectors, which is a vector that is perpendicular to both original vectors and has a magnitude equal to the area of the parallelogram formed by the two vectors
	//
	inline static Vector cross(const Vector& vec1, const Vector& vec2) {
		return vec1.cross(vec2);
	}

	//
	//  FUNCTION: Vector::dot(Vector)
	//
	//  PURPOSE: Returns the dot product of this vector with the given other vector, which is a scalar equal to the sum of the products of the corresponding components of the two vectors; it can be used to determine the angle between the vectors or to project one vector onto another
	//
	inline double dot(const Vector& other) const {
		return this->getX() * other.getX() + this->getY() * other.getY() + this->getZ() * other.getZ();
	}

	//
	//  FUNCTION: Vector::dot(Vector, Vector)
	//
	//  PURPOSE: Returns the dot product of the two given vectors, which is a scalar equal to the sum of the products of the corresponding components of the two vectors; it can be used to determine the angle between the vectors or to project one vector onto another
	//
	inline static double dot(const Vector& vec1, const Vector& vec2) {
		return vec1.dot(vec2);
	}

	//
	//  FUNCTION: Vector::magnitude()
	//
	//  PURPOSE: Returns the magnitude of the vector, which is the square root of the sum of the squares of its components; this represents the length of the vector
	//
	inline double magnitude() const {
		return sqrt(getX() * getX() + getY() * getY() + getZ() * getZ());
	}

	//
	//  FUNCTION: Vector::normalize()
	//
	//  PURPOSE: Returns a new vector that has the same direction as this vector but a magnitude of 1, which is done by dividing each component of the vector by its magnitude; this is useful for representing directions without regard to length
	//
	inline Vector normalize() const {
		double sum = magnitude();
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

	inline Vector operator*(const double& scalar) const {
		return Vector(asPoint() * scalar);
	}

	inline Vector operator*=(const double& other) {
		this->x = this->getX() * other;
		this->y = this->getY() * other;
		this->z = this->getZ() * other;
		return Vector(this->x, this->y, this->z);
	}

	inline friend Vector operator*(const double& scalar, const Vector& vector) {
		return Vector(vector.asPoint() * scalar);
	}

	inline Vector operator/(const double& scalar) const {
		return Vector(asPoint() / scalar);
	}

	inline Vector operator/=(const double& scalar) {
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

	operator Point3D() const {
		return Point3D(getX(), getY(), getZ());
	}


	//Get Methods
	inline double getX() const {
		return this->x;
	}

	inline double getY() const {
		return this->y;
	}

	inline double getZ() const {
		return this->z;
	}

	//Legacy, don't use this, just use the Point3D constructor instead, but this is here for backward compatibility with old code that expects a Point3D conversion method asPoint()
	inline Point3D asPoint() const {
		return Point3D(getX(), getY(), getZ());
	}
};