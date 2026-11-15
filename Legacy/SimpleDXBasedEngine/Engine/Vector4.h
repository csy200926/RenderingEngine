#pragma once
#include "Extension.h"
namespace MyEngine
{
class Vector4
{
public:

	float _x, _y ,_z,_a;

	Vector4(const float x,const float y,const float z,const float a);
	Vector4(void);

	~Vector4(void);
	
	// comparison 
	bool operator==(const Vector4 &v2)const;
		// operators
	void operator= (const Vector4 & i_other);
	const Vector4 operator+(const Vector4 &v2);
	const Vector4 operator-(const Vector4 &v2);
	const Vector4 operator*(float times);
	
	void operator+=(const Vector4 &v2);
	void operator-=(const Vector4 &v2);
	void operator*=(const float times);

	inline float getX()const;
	inline float getY()const;
	inline float getZ()const;

	inline void printOut()const;

	inline void setValue(const float x,const float y,const float z);
};

	void Vector4::setValue(const float x,const float y,const float z)
	{

		this->_x = x;
		this->_y = y;
		this->_z = z;
	}

	float Vector4::getX()const
	{
		return _x;
	}
	float Vector4::getY()const
	{
		return _y;
	}
	float Vector4::getZ()const
	{
		return _z;
	}

	void Vector4::printOut()const
	{
		DEBUG_PRINT("x = %f ,y = %f , z = %f",_x,_y,_z);
	}

}