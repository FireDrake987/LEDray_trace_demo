#pragma once
#include "Vector.h"
class Quaternion {
private: 
	double w, x, y, z;
public: 
	//
	//  FUNCTION: Quaternion::Quaternion(double, double, double, double)
	//
	//  PURPOSE: Initializes a quaternion with the given raw w/x/y/z values
	//
	Quaternion(double w, double x, double y, double z);

	//
	//  FUNCTION: Quaternion::Quaternion(double, Vector)
	//  
	//  PURPOSE: Initializes a quaternion with the given w value and vector part; w=cos(theta/2), vector=axis*sin(theta/2)
	//
	Quaternion(double w, Vector vec);

	//
	//  FUNCTION: Quaternion::Quaternion(Vector)
	//
	//  PURPOSE: Initializes a quaternion with the given vector part; w=0, vector=vec
	//
	Quaternion(Vector vec);

	//
	//  FUNCTION: Quaternion::Quaternion()
	//
	//  PURPOSE: Initializes a quaternion with w=1 and x/y/z=0, representing the identity rotation
	//
	Quaternion();


	//
	//  FUNCTION: Quaternion::conjugate()
	//
	//  PURPOSE: Returns the conjugate of the quaternion, which is the same as the original but with the vector part negated; useful for applying rotations
	//
	inline Quaternion conjugate() const {
		return Quaternion(getW(), -getX(), -getY(), -getZ());
	}

	//
	//  FUNCTION: Quaternion::apply(Point3D, Quaternion)
	//
	//  PURPOSE: Applies the rotation represented by the quaternion to the given point, returning the rotated point; this is done by treating the point as a quaternion with w=0 and applying the formula q * p * q*
	//
	inline static Point3D apply(const Point3D& point, const Quaternion& qua) {
		return apply(Quaternion(point), qua);
	}

	//
	//  FUNCTION: Quaternion::apply(Quaternion, Quaternion)
	//
	//  PURPOSE: Applies the rotation represented by the second quaternion to the point represented by the first quaternion, returning the rotated point; this is done by applying the formula q * p * q*
	//
	inline static Point3D apply(const Quaternion& point, const Quaternion& qua) {
		const Quaternion normQua = qua.normalize();
		Quaternion endQua = normQua * point * normQua.conjugate();
		return Point3D(endQua.getX(), endQua.getY(), endQua.getZ());
	}

	//
	//  FUNCTION: Quaternion::apply(Point3D)
	//
	//  PURPOSE: Applies the rotation represented by this quaternion to the given point, returning the rotated point; this is done by treating the point as a quaternion with w=0 and applying the formula q * p * q*
	inline Point3D apply(const Point3D& point) const {
		return apply(point, *this);
	}

	//
	//  FUNCTION: Quaternion::magnitude()
	//
	//  PURPOSE: Returns the magnitude of the quaternion, which is the square root of the sum of the squares of its components; for rotation quaternions, this should be 1
	//
	//  COMMENTS:
	// 
	//       If the magnitude is not 1, the quaternion may not represent a pure rotation and may need to be normalized before use
	//
	inline double magnitude() const {
		return sqrt(getW() * getW() + getX() * getX() + getY() * getY() + getZ() * getZ());
	}

	//
	//  FUNCTION: Quaternion::normalize()
	//
	//  PURPOSE: Returns a normalized version of the quaternion, which is the same quaternion scaled to have a magnitude of 1; this is done by dividing each component by the magnitude
	//
	inline Quaternion normalize() const {
		double sum = magnitude();
		return Quaternion(getW()/sum, getX()/sum, getY()/sum, getZ()/sum);
	}


	//Operators
	inline Quaternion operator*(const Quaternion& other) const {
		return Quaternion(
			this->getW() * other.getW() - this->getX() * other.getX() - this->getY() * other.getY() - this->getZ() * other.getZ(), 
			this->getW() * other.getX() + this->getX() * other.getW() + this->getY() * other.getZ() - this->getZ() * other.getY(), 
			this->getW() * other.getY() - this->getX() * other.getZ() + this->getY() * other.getW() + this->getZ() * other.getX(), 
			this->getW() * other.getZ() + this->getX() * other.getY() - this->getY() * other.getX() + this->getZ() * other.getW()
		);
	}

	inline friend std::ostream& operator<<(std::ostream& os, const Quaternion& quaternion) {
		os << quaternion.getW() << (quaternion.getX() < 0 ? "" : "+") << quaternion.getX() << "i" << (quaternion.getY() < 0 ? "" : "+") << quaternion.getY() << "j" << (quaternion.getZ() < 0 ? "" : "+") << quaternion.getZ() << "k";
		return os;
	}

	inline bool operator==(const Quaternion& other) const {
		return (getW() == other.getW() && getX() == other.getX() && getY() == other.getY() && getZ() == other.getZ());
	}

	inline bool operator!=(const Quaternion& other) {
		return !(*this == other);
	}


	//Get Methods
	inline double getW() const {
		return this->w;
	}

	inline double getX() const {
		return this->x;
	}

	inline double getY() const {
		return this->y;
	}

	inline double getZ() const {
		return this->z;
	}
};