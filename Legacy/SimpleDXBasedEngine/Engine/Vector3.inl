#pragma once
#include"Vector3.h"
#include "math.h"
#include <assert.h>
#include "MathStuff.h"
#include "Extension.h"
namespace MyEngine{

	float Vector3::dot(const Vector3 &v)
	{
		assert(!IsNAN(v.getX())||!IsNAN(v.getY())||!IsNAN(v.getZ()));

		float result;
		result = _x * v.getX() + _y * v.getY() + _z * v.getZ();
		return result;
	}

	float Vector3::length() 
	{
		float result;
		result = sqrt(pow(_x,2) + pow(_y,2) + pow(_z,2));
		return result;
	}

	float Vector3::getX()const
	{
		return _x;
	}
	float Vector3::getY()const
	{
		return _y;
	}
	float Vector3::getZ()const
	{
		return _z;
	}

	void Vector3::setValue(const float x,const float y,const float z)
	{
		assert(!IsNAN(x)||!IsNAN(y)||!IsNAN(z));

		this->_x = x;
		this->_y = y;
		this->_z = z;
	}

	void Vector3::clear()
	{
		this->_x = 0.0f;
		this->_y = 0.0f;
		this->_z = 0.0f;

	}
	

	void Vector3::printOut()const
	{
		DEBUG_PRINT("x = %f ,y = %f , z = %f \n",_x,_y,_z);
	}
}