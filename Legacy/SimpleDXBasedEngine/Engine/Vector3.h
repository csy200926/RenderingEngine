#pragma once

namespace MyEngine{

	class Vector3
	{

	private:


		float _x, _y ,_z;


	public:

		Vector3();
		Vector3(const float x,const float y,const float z);
		Vector3(const Vector3 & i_other);
		~Vector3(void);

		// Basic operations
		inline void setValue(const float x,const float y,const float z);
		inline void clear();

		inline float getX()const;
		inline float getY()const;
		inline float getZ()const;

		inline float length();

		void setX(const float x);
		void setY(const float y);
		void setZ(const float z);

		inline void printOut()const;

		// operators
		void operator= (const Vector3 & i_other);
		const Vector3 operator+(const Vector3 &v2);
		const Vector3 operator-(const Vector3 &v2);
		const Vector3 operator*(float times);

		const Vector3 cross(const Vector3 &v);
		inline float dot(const Vector3 &v);

		void operator+=(const Vector3 &v2);
		void operator-=(const Vector3 &v2);
		void operator*=(const float times);

		// comparison 
		bool operator==(const Vector3 &v2)const;


	};
}

#include "Vector3.inl"

