#pragma once
#include "Vector4.h"

namespace MyEngine{


	class Matrix
	{

	private:

		float m_Values[4][4];

	public:

		#define PI 3.1415926
		#define DEG_TO_RADIANS(x)  x * PI/180

		Matrix( );
		Matrix( float i_Values[4][4] );
		Matrix(float i_f00,float i_f01,float i_f02,float i_f03,
					float i_f10,float i_f11,float i_f12,float i_f13,
					float i_f20,float i_f21,float i_f22,float i_f23,
					float i_f30,float i_f31,float i_f32,float i_f33);
		Matrix(const Matrix &i_other);
		~Matrix(void);
		Matrix &operator = (const Matrix &i_other);
		bool operator==(const Matrix &i_other)const;
		
		void Identity(void);
		void Transpose(void);
		void Inverse(void);

		float determinant(void);

		void CreateRotateX( float i_rotation );
		void CreateRotateY( float i_rotation );
		void CreateRotateZ( float i_rotation );


		void Translate( float i_X,float i_Y,float i_Z );

		Matrix operator *(const Matrix & i_other);

		Vector4 MultiplyMAtLeft( const Vector4 & i_Vector ) const;
		
		Vector4 MultiplyMAtRight( const Vector4 & i_Vector ) const;

		void print();

	};
	Vector4 operator*( const Matrix & i_Matrix, const Vector4 & i_Vector4 );
	Vector4 operator*( const Vector4 & i_Vector4, const Matrix & i_Matrix );
}