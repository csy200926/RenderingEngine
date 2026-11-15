#include "Matrix.h"
#include "Vector4.h"
#include <math.h>
#include "Extension.h"
#include "MathStuff.h"
namespace MyEngine
{

	Matrix::Matrix()
	{



	}

	Matrix::Matrix(float i_f00,float i_f01,float i_f02,float i_f03,
					float i_f10,float i_f11,float i_f12,float i_f13,
					float i_f20,float i_f21,float i_f22,float i_f23,
					float i_f30,float i_f31,float i_f32,float i_f33)
	{
		m_Values[0][0] = i_f00;m_Values[0][1] = i_f01;m_Values[0][2] = i_f02;m_Values[0][3] = i_f03;
		m_Values[1][0] = i_f10;m_Values[1][1] = i_f11;m_Values[1][2] = i_f12;m_Values[1][3] = i_f13;
		m_Values[2][0] = i_f20;m_Values[2][1] = i_f21;m_Values[2][2] = i_f22;m_Values[2][3] = i_f23;
		m_Values[3][0] = i_f30;m_Values[3][1] = i_f31;m_Values[3][2] = i_f32;m_Values[3][3] = i_f33;


	}

	
	Matrix::Matrix(const Matrix &i_other)
	{
		for (int  i = 0; i < 4; i++)
		{
			for (int  j = 0; j < 4; j++)
			{
				m_Values[i][j] = i_other.m_Values[i][j];
			}
		}
	
	
	}

	Matrix & Matrix::operator = (const Matrix &i_other)
	{
		for (int  i = 0; i < 4; i++)
		{
			for (int  j = 0; j < 4; j++)
			{
				m_Values[i][j] = i_other.m_Values[i][j];
			}
		}

		return *this;
	
	}

	Matrix::Matrix( float i_Values[4][4] )
	{
		for (int  i = 0; i < 4; i++)
		{
			for (int  j = 0; j < 4; j++)
			{
				m_Values[i][j] = i_Values[i][j];
			}
		}
	
	}

	void Matrix::Identity(void)
	{
	
		int i,j;
		for ( i = 0; i < 4; i++)
		{
			for ( j = 0; j < 4; j++)
			{
				//DEBUG_PRINT("%d,%d",i,j);
				if ( i == j )
					m_Values[i][j] = 1.0f;
				else
					m_Values[i][j] = 0.0f;
			}
		}
	}

	void Matrix::Transpose(void)
	{
		int i,j;
		for ( i = 0; i < 4; i++)
		{
			for ( j = i; j < 4; j++)
			{
				float temp = m_Values[i][j];
				m_Values[i][j] = m_Values[j][i];
				m_Values[j][i] = temp;

			}
		}
	
	}

	void Matrix::Inverse(void)
	{
		float result[4][4];


		result[0][0] = m_Values[1][1]*m_Values[2][2]*m_Values[3][3]
		              +m_Values[1][2]*m_Values[2][3]*m_Values[3][1]
					  +m_Values[1][3]*m_Values[2][1]*m_Values[3][2]
					  -m_Values[1][1]*m_Values[2][3]*m_Values[3][2]
					  -m_Values[1][2]*m_Values[2][1]*m_Values[3][3]
					  -m_Values[1][3]*m_Values[2][2]*m_Values[3][1];
		result[0][1] = m_Values[0][1]*m_Values[2][3]*m_Values[3][2]
		              +m_Values[0][2]*m_Values[2][1]*m_Values[3][3]
					  +m_Values[0][3]*m_Values[2][2]*m_Values[3][1]
					  -m_Values[0][1]*m_Values[2][2]*m_Values[3][3]
					  -m_Values[0][2]*m_Values[2][3]*m_Values[3][1]
					  -m_Values[0][3]*m_Values[2][1]*m_Values[3][2];
		result[0][2] = m_Values[0][1]*m_Values[1][2]*m_Values[3][3]
		              +m_Values[0][2]*m_Values[1][3]*m_Values[3][1]
					  +m_Values[0][3]*m_Values[1][1]*m_Values[3][2]
					  -m_Values[0][1]*m_Values[1][3]*m_Values[3][2]
					  -m_Values[0][2]*m_Values[1][1]*m_Values[3][3]
					  -m_Values[0][3]*m_Values[1][2]*m_Values[3][1];
		result[0][3] = m_Values[0][1]*m_Values[1][3]*m_Values[2][2]
		              +m_Values[0][2]*m_Values[1][1]*m_Values[2][3]
					  +m_Values[0][3]*m_Values[1][2]*m_Values[2][1]
					  -m_Values[0][1]*m_Values[1][2]*m_Values[2][3]
					  -m_Values[0][2]*m_Values[1][3]*m_Values[2][1]
					  -m_Values[0][3]*m_Values[1][1]*m_Values[2][2];

		result[1][0] = m_Values[1][0]*m_Values[2][3]*m_Values[3][2]
		              +m_Values[1][2]*m_Values[2][0]*m_Values[3][3]
					  +m_Values[1][3]*m_Values[2][2]*m_Values[3][0]
					  -m_Values[1][0]*m_Values[2][2]*m_Values[3][3]
					  -m_Values[1][2]*m_Values[2][3]*m_Values[3][0]
					  -m_Values[1][3]*m_Values[2][0]*m_Values[3][2];
		result[1][1] = m_Values[0][0]*m_Values[2][2]*m_Values[3][3]
		              +m_Values[0][2]*m_Values[2][3]*m_Values[3][0]
					  +m_Values[0][3]*m_Values[2][0]*m_Values[3][2]
					  -m_Values[0][0]*m_Values[2][3]*m_Values[3][2]
					  -m_Values[0][2]*m_Values[2][0]*m_Values[3][3]
					  -m_Values[0][3]*m_Values[2][2]*m_Values[3][0];
		result[1][2] = m_Values[0][0]*m_Values[1][3]*m_Values[3][2]
		              +m_Values[0][2]*m_Values[1][0]*m_Values[3][3]
					  +m_Values[0][3]*m_Values[1][2]*m_Values[3][0]
					  -m_Values[0][0]*m_Values[1][2]*m_Values[3][3]
					  -m_Values[0][2]*m_Values[1][3]*m_Values[3][0]
					  -m_Values[0][3]*m_Values[1][0]*m_Values[3][2];
		result[1][3] = m_Values[0][0]*m_Values[1][2]*m_Values[2][3]
		              +m_Values[0][2]*m_Values[1][3]*m_Values[2][0]
					  +m_Values[0][3]*m_Values[1][0]*m_Values[2][2]
					  -m_Values[0][0]*m_Values[1][3]*m_Values[2][2]
					  -m_Values[0][2]*m_Values[1][0]*m_Values[2][3]
					  -m_Values[0][3]*m_Values[1][2]*m_Values[2][0];

		result[2][0] = m_Values[1][0]*m_Values[2][1]*m_Values[3][3]
		              +m_Values[1][1]*m_Values[2][3]*m_Values[3][0]
					  +m_Values[1][3]*m_Values[2][0]*m_Values[3][1]
					  -m_Values[1][0]*m_Values[2][3]*m_Values[3][1]
					  -m_Values[1][1]*m_Values[2][0]*m_Values[3][3]
					  -m_Values[1][3]*m_Values[2][1]*m_Values[3][0];
		result[2][1] = m_Values[0][0]*m_Values[2][3]*m_Values[3][1]
		              +m_Values[0][1]*m_Values[2][0]*m_Values[3][3]
					  +m_Values[0][3]*m_Values[2][1]*m_Values[3][0]
					  -m_Values[0][0]*m_Values[2][1]*m_Values[3][3]
					  -m_Values[0][1]*m_Values[2][3]*m_Values[3][0]
					  -m_Values[0][3]*m_Values[2][0]*m_Values[3][1];
		result[2][2] = m_Values[0][0]*m_Values[1][1]*m_Values[3][3]
		              +m_Values[0][1]*m_Values[1][3]*m_Values[3][0]
					  +m_Values[0][3]*m_Values[1][0]*m_Values[3][1]
					  -m_Values[0][0]*m_Values[1][3]*m_Values[3][1]
					  -m_Values[0][1]*m_Values[1][0]*m_Values[3][3]
					  -m_Values[0][3]*m_Values[1][1]*m_Values[3][0];
		result[2][3] = m_Values[0][0]*m_Values[1][3]*m_Values[2][1]
		              +m_Values[0][1]*m_Values[1][0]*m_Values[2][3]
					  +m_Values[0][3]*m_Values[1][1]*m_Values[2][0]
					  -m_Values[0][0]*m_Values[1][1]*m_Values[2][3]
					  -m_Values[0][1]*m_Values[1][3]*m_Values[2][0]
					  -m_Values[0][3]*m_Values[1][0]*m_Values[2][1];

		result[3][0] = m_Values[1][0]*m_Values[2][2]*m_Values[3][1]
		              +m_Values[1][1]*m_Values[2][0]*m_Values[3][2]
					  +m_Values[1][2]*m_Values[2][1]*m_Values[3][0]
					  -m_Values[1][0]*m_Values[2][1]*m_Values[3][2]
					  -m_Values[1][1]*m_Values[2][2]*m_Values[3][0]
					  -m_Values[1][2]*m_Values[2][0]*m_Values[3][1];
		result[3][1] = m_Values[0][0]*m_Values[2][1]*m_Values[3][2]
		              +m_Values[0][1]*m_Values[2][2]*m_Values[3][0]
					  +m_Values[0][2]*m_Values[2][0]*m_Values[3][1]
					  -m_Values[0][0]*m_Values[2][2]*m_Values[3][1]
					  -m_Values[0][1]*m_Values[2][0]*m_Values[3][2]
					  -m_Values[0][2]*m_Values[2][1]*m_Values[3][0];
		result[3][2] = m_Values[0][0]*m_Values[1][2]*m_Values[3][1]
		              +m_Values[0][1]*m_Values[1][0]*m_Values[3][2]
					  +m_Values[0][2]*m_Values[1][1]*m_Values[3][0]
					  -m_Values[0][0]*m_Values[1][1]*m_Values[3][2]
					  -m_Values[0][1]*m_Values[1][2]*m_Values[3][0]
					  -m_Values[0][2]*m_Values[1][0]*m_Values[3][1];
		result[3][3] = m_Values[0][0]*m_Values[1][1]*m_Values[2][2]
		              +m_Values[0][1]*m_Values[1][2]*m_Values[2][0]
					  +m_Values[0][2]*m_Values[1][0]*m_Values[2][1]
					  -m_Values[0][0]*m_Values[1][2]*m_Values[2][1]
					  -m_Values[0][1]*m_Values[1][0]*m_Values[2][2]
					  -m_Values[0][2]*m_Values[1][1]*m_Values[2][0];

		float det = determinant();

		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				m_Values[i][j] = result[i][j] / det;
			}
		}

	}

	float determinantOf3x3(float i_input[3][3])
	{
		float result = 0.0f;

		result += i_input[0][0] * ( i_input[1][1]*i_input[2][2] - i_input[2][1]*i_input[1][2] );
		result -= i_input[0][1] * ( i_input[1][0]*i_input[2][2] - i_input[2][0]*i_input[1][2] );
		result += i_input[0][2] * ( i_input[1][0]*i_input[2][1] - i_input[1][1]*i_input[2][0] );
	
		return result;
	}


	float Matrix::determinant(void)
	{
		float result = 0.0f;

		float matrix3x3Block_1[3][3] = {
			{m_Values[1][1],m_Values[1][2],m_Values[1][3]},
			{m_Values[2][1],m_Values[2][2],m_Values[2][3]},
			{m_Values[3][1],m_Values[3][2],m_Values[3][3]}
		};
		float matrix3x3Block_2[3][3] = {
			{m_Values[1][0],m_Values[1][2],m_Values[1][3]},
			{m_Values[2][0],m_Values[2][2],m_Values[2][3]},
			{m_Values[3][0],m_Values[3][2],m_Values[3][3]}
		};
		float matrix3x3Block_3[3][3] = {
			{m_Values[1][0],m_Values[1][1],m_Values[1][3]},
			{m_Values[2][0],m_Values[2][1],m_Values[2][3]},
			{m_Values[3][0],m_Values[3][1],m_Values[3][3]}
		};
		float matrix3x3Block_4[3][3] = {
			{m_Values[1][0],m_Values[1][1],m_Values[1][2]},
			{m_Values[2][0],m_Values[2][1],m_Values[2][2]},
			{m_Values[3][0],m_Values[3][1],m_Values[3][2]}
		};

		result = determinantOf3x3(matrix3x3Block_1) * m_Values[0][0] 
			   - determinantOf3x3(matrix3x3Block_2) * m_Values[0][1]
			   + determinantOf3x3(matrix3x3Block_3) * m_Values[0][2]
			   - determinantOf3x3(matrix3x3Block_4) * m_Values[0][3];


		return result;
	
	}

	void Matrix::CreateRotateX( float i_rotation )
	{
		m_Values[0][0] = 1;m_Values[0][1] = 0;m_Values[0][2] = 0;m_Values[0][3] = 0;
		m_Values[1][0] = 0;m_Values[1][1] = cos(i_rotation);m_Values[1][2] = -sin(i_rotation);m_Values[1][3] = 0;
		m_Values[2][0] = 0;m_Values[2][1] = sin(i_rotation);m_Values[2][2] = cos(i_rotation);m_Values[2][3] = 0;
		m_Values[3][0] = 0;m_Values[3][1] = 0;m_Values[3][2] = 0;m_Values[3][3] = 1;
	
	}
	void Matrix::CreateRotateY( float i_rotation )
	{
		m_Values[0][0] = cos(i_rotation);m_Values[0][1] = 0;m_Values[0][2] = sin(i_rotation);m_Values[0][3] = 0;
		m_Values[1][0] = 0;m_Values[1][1] = 1;m_Values[1][2] = 0;m_Values[1][3] = 0;
		m_Values[2][0] = -sin(i_rotation);m_Values[2][1] = 0;m_Values[2][2] = cos(i_rotation);m_Values[2][3] = 0;
		m_Values[3][0] = 0;m_Values[3][1] = 0;m_Values[3][2] = 0;m_Values[3][3] = 1;
	
	}
	void Matrix::CreateRotateZ( float i_rotation )
	{
		m_Values[0][0] = cos(i_rotation);m_Values[0][1] = -sin(i_rotation);m_Values[0][2] = 0;m_Values[0][3] = 0;
		m_Values[1][0] = sin(i_rotation);m_Values[1][1] = cos(i_rotation);m_Values[1][2] = 0;m_Values[1][3] = 0;
		m_Values[2][0] = 0;m_Values[2][1] = 0;m_Values[2][2] = 1;m_Values[2][3] = 0;
		m_Values[3][0] = 0;m_Values[3][1] = 0;m_Values[3][2] = 0;m_Values[3][3] = 1;
	
	}

	void Matrix::Translate( float i_X,float i_Y,float i_Z )
	{
	
		m_Values[0][0] = 1;m_Values[0][1] = 0;m_Values[0][2] = 0;m_Values[0][3] = i_X;
		m_Values[1][0] = 0;m_Values[1][1] = 1;m_Values[1][2] = 0;m_Values[1][3] = i_Y;
		m_Values[2][0] = 0;m_Values[2][1] = 0;m_Values[2][2] = 1;m_Values[2][3] = i_Z;
		m_Values[3][0] = 0;m_Values[3][1] = 0;m_Values[3][2] = 0;m_Values[3][3] = 1;
	}

	void Matrix::print(void)
	{
		DEBUG_PRINT("\n");
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				DEBUG_PRINT(" %f",m_Values[i][j]);
				if (j == 3)
					DEBUG_PRINT("\n");
				
			}
		}
		
	
	}

	Matrix Matrix::operator*(const Matrix & i_other) 
	{
		float result[4][4];
		int i = 0,j = 0;
		for (; i < 4; i++)
		{
			for ( j = 0; j < 4; j++)
			{
				result[i][j] = 0;

				for (int n = 0; n < 4; n++)
				{
					result[i][j] += (m_Values[i][n] * i_other.m_Values[n][j]);
				}

			}
		}

		Matrix resultMatrix(result);

		return resultMatrix;
	}


	


	Vector4 Matrix::MultiplyMAtLeft( const Vector4 & i_Vector ) const
	{
					
		float x,y,z;

		x = m_Values[0][0] * i_Vector.getX() + m_Values[0][1] * i_Vector.getY() + m_Values[0][2] * i_Vector.getZ() + m_Values[0][3];
		y = m_Values[1][0] * i_Vector.getX() + m_Values[1][1] * i_Vector.getY() + m_Values[1][2] * i_Vector.getZ() + m_Values[1][3];
		z = m_Values[2][0] * i_Vector.getX() + m_Values[2][1] * i_Vector.getY() + m_Values[2][2] * i_Vector.getZ() + m_Values[2][3];
	
		Vector4 result(x,y,z,1);
		return result;


	}

	Vector4 Matrix::MultiplyMAtRight( const Vector4 & i_Vector ) const
	{
			
		float x,y,z;

		x = m_Values[0][0] * i_Vector.getX() + m_Values[0][1] * i_Vector.getY() + m_Values[0][2] * i_Vector.getZ() + m_Values[0][3];
		y = m_Values[1][0] * i_Vector.getX() + m_Values[1][1] * i_Vector.getY() + m_Values[1][2] * i_Vector.getZ() + m_Values[1][3];
		z = m_Values[2][0] * i_Vector.getX() + m_Values[2][1] * i_Vector.getY() + m_Values[2][2] * i_Vector.getZ() + m_Values[2][3];
	
		Vector4 result(x,y,z,1);
		return result;
	}
	

	Vector4 operator*( const Matrix & i_Matrix, const Vector4 & i_Vector4 )
	{
		return i_Matrix.MultiplyMAtLeft(i_Vector4);
	}

	Vector4 operator*( const Vector4 & i_Vector4, const Matrix & i_Matrix )
	{
	
		return i_Matrix.MultiplyMAtLeft(i_Vector4);
	}

	bool Matrix::operator==(const Matrix &i_other)const
	{

		for (int  i = 0; i < 4; i++)
		{
			for (int  j = 0; j < 4; j++)
			{
				if(!AlmostEqualRelative(m_Values[i][j],i_other.m_Values[i][j]))
					return false;
			}
		}

		return true;

	
	}

	Matrix::~Matrix(void)
	{

	}
}