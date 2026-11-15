#include "Vector4.h"
#include "MathStuff.h"
#include <assert.h>
namespace MyEngine{

	Vector4::Vector4(const float i_x,const float i_y,const float i_z,const float i_a):
		_x(i_x),
		_y(i_y),
		_z(i_z),
		_a(i_a)
	{
		
	}
	Vector4::Vector4()
	{
		
	}

	Vector4::~Vector4(void)
	{
	}

	bool Vector4::operator==(const Vector4 &v2)const
	{
		

		if (!AlmostEqualRelative(_x,v2.getX()))
			return false;
		if (!AlmostEqualRelative(_y,v2.getY()))
			return false;
		if (!AlmostEqualRelative(_z,v2.getZ()))
			return false;

		return true;
	}
	
	void Vector4::operator= (const Vector4 & i_other)
	{
		
		_x = i_other.getX();
		_y = i_other.getY();
		_z = i_other.getZ();

	}

	const Vector4 Vector4::operator+(const Vector4 &v2)
	{
		assert(!IsNAN(v2.getX())||!IsNAN(v2.getY())||!IsNAN(v2.getZ()));

		Vector4 result;

		result.setValue(_x + v2.getX(),_y + v2.getY(),_z + v2.getZ());

		return result;
	}

	const Vector4 Vector4::operator-(const Vector4 &v2)
	{
		assert(!IsNAN(v2.getX())||!IsNAN(v2.getY())||!IsNAN(v2.getZ()));

		Vector4 result;

		result.setValue(_x - v2.getX(),_y - v2.getY(),_z - v2.getZ());

		return result;
	}


	const Vector4 Vector4::operator*(const float times)
	{
		

		Vector4 result;

		result.setValue(times * _x,times * _y,times * _z);

		return result;
	}

	void Vector4::operator+=(const Vector4 &v2)
	{
		assert(!IsNAN(v2.getX())||!IsNAN(v2.getY())||!IsNAN(v2.getZ()));

		_x += v2.getX();
		_y += v2.getY();
		_z += v2.getZ();

	}

	void Vector4::operator-=(const Vector4 &v2)
	{
		assert(!IsNAN(v2.getX())||!IsNAN(v2.getY())||!IsNAN(v2.getZ()));
		_x -= v2.getX();
		_y -= v2.getY();
		_z -= v2.getZ();
	}

	void Vector4::operator*=(float times)
	{
		assert(!IsNAN(times));

		_x *= times;
		_y *= times;
		_z *= times;
	}



}