#include "Vector3.h"
//#include <iostream>
#include "MathStuff.h"
#include <assert.h> 

namespace MyEngine{

	Vector3::Vector3()
	{


	}

	Vector3::Vector3(const float x,const float y,const float z):
		_x(x),
		_y(y),
		_z(z)
	{
		assert(!IsNAN(x)||!IsNAN(y)||!IsNAN(z));

	}
	Vector3::Vector3(const Vector3 & i_other):
		_x(i_other.getX()),
		_y(i_other.getY()),
		_z(i_other.getZ())
	{
		assert(!IsNAN(i_other.getX())||!IsNAN(i_other.getY())||!IsNAN(i_other.getZ()));

	}

	Vector3::~Vector3(void)
	{
	}

	const Vector3 Vector3::cross(const Vector3 &v)
	{
		assert(!IsNAN(v.getX())||!IsNAN(v.getY())||!IsNAN(v.getZ()));

		Vector3 result;

		float x,y,z;

		x = _y * v.getZ() - _z * v.getY();
		y = _z * v.getX() - _x * v.getZ();
		z = _x * v.getY() - _y * v.getX();

		result.setValue(x,y,z);

		return result;
	}



	void Vector3::operator= (const Vector3 & i_other)
	{
		assert(!IsNAN(i_other.getX())||!IsNAN(i_other.getY())||!IsNAN(i_other.getZ()));
		_x = i_other.getX();
		_y = i_other.getY();
		_z = i_other.getZ();

	}

	const Vector3 Vector3::operator+(const Vector3 &v2)
	{
		assert(!IsNAN(v2.getX())||!IsNAN(v2.getY())||!IsNAN(v2.getZ()));

		Vector3 result;

		result.setValue(_x + v2.getX(),_y + v2.getY(),_z + v2.getZ());

		return result;
	}

	const Vector3 Vector3::operator-(const Vector3 &v2)
	{
		assert(!IsNAN(v2.getX())||!IsNAN(v2.getY())||!IsNAN(v2.getZ()));

		Vector3 result;

		result.setValue(_x - v2.getX(),_y - v2.getY(),_z - v2.getZ());

		return result;
	}


	const Vector3 Vector3::operator*(const float times)
	{
		assert(!IsNAN(times));

		Vector3 result;

		result.setValue(times * _x,times * _y,times * _z);

		return result;
	}

	void Vector3::operator+=(const Vector3 &v2)
	{
		assert(!IsNAN(v2.getX())||!IsNAN(v2.getY())||!IsNAN(v2.getZ()));

		_x += v2.getX();
		_y += v2.getY();
		_z += v2.getZ();

	}

	void Vector3::operator-=(const Vector3 &v2)
	{
		assert(!IsNAN(v2.getX())||!IsNAN(v2.getY())||!IsNAN(v2.getZ()));
		_x -= v2.getX();
		_y -= v2.getY();
		_z -= v2.getZ();
	}

	void Vector3::operator*=(float times)
	{
		assert(!IsNAN(times));

		_x *= times;
		_y *= times;
		_z *= times;
	}




	void Vector3::setX(float x)
	{
		assert(!IsNAN(x));
		this->_x = x;
	}
	void Vector3::setY(float y)
	{
		assert(!IsNAN(y));
		this->_y = y;
	}
	void Vector3::setZ(float z)
	{
		assert(!IsNAN(z));
		this->_z = z;
	}



	bool Vector3::operator==(const Vector3 &v2)const
	{
		assert(!IsNAN(v2.getX())||!IsNAN(v2.getY())||!IsNAN(v2.getZ()));

		if (!AlmostEqualRelative(_x,v2.getX()))
			return false;
		if (!AlmostEqualRelative(_y,v2.getY()))
			return false;
		if (!AlmostEqualRelative(_z,v2.getZ()))
			return false;

		return true;
	}
}