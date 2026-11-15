#include "UnitTest.h"
#include "MathStuff.h"
#include "RingBuffer.h"
#include "RingBuffer.cpp"
#include "assert.h"
#include <iostream>
#include "Matrix.h"
#include "Vector4.h"


using namespace std;
namespace MyEngine{
UnitTest::UnitTest(void)
{


}

UnitTest::~UnitTest(void)
{


}
bool UnitTest::runTest()
{


	if (IdentityTest() && TransposeTest() && InverseTest() && 
	CreateRotateTest() && TranslateTest() && multiplyTest())
	{
		return true;
	}

	return false;


}
bool UnitTest::IdentityTest(void)
{
	// expected result
	Matrix target(
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1
		);


	// Testing
	Matrix result;
	result.Identity();

	if (target == result)
	{
		return true;//IdentityTest
	}
	return false;


}
bool UnitTest::TransposeTest(void)
{
	// expected result
	Matrix target(
	1,0,0,0,
	1,1,0,0,
	1,1,1,0,
	1,1,1,1
	);

	//Testing
	Matrix result(
	1,1,1,1,
	0,1,1,1,
	0,0,1,1,
	0,0,0,1
	);

	result.Transpose();

	if (target == result)
	{
		return true;//TransposeTest

	}
	return false;
}
bool UnitTest::InverseTest(void)
{

	Matrix target(
	1,0,0,0,
	2,1,0,0,
	2,1,1,0,
	2,1,1,1
	);

	Matrix target2(target);
	target2.Inverse();
	
	Matrix result = (target * target2);

	Matrix unit;
	unit.Identity();

	if (result == unit)
	{
		return true;//InverseTest
	}
	return false;

}



bool UnitTest::CreateRotateTest( void )
{
	//***** Calculated with pen  Calculated with pen   Calculated with pen
	Vector4 target( 0.0f, 1.0f, 0.0f , 1.0f );


	// Testing
	Vector4 result( 1.0f, 0.0f, 0.0f , 1.0f );
	Matrix Rotation;

	Rotation.CreateRotateZ((float)( DEG_TO_RADIANS(90) )); 
	result =  result * Rotation;


	if (target == result)
	{
		return true;//CreateRotateTest
	}
	return false;
}


bool UnitTest::TranslateTest( void )
{
	//****** Calculated with pen  Calculated with pen   Calculated with pen
	Vector4 target( 2.0f, 2.3f, 10.0f , 1.0f );
	

	// Testing
	Vector4 result( 1.0f, 0.0f, 0.0f , 1.0f );

	Matrix Translation;
	Translation.Translate( 1.0f, 2.3f, 10.0f );

	result =  result * Translation;
	if (target == result)
	{
		return true;//TranslateTest
	}
	return false;

}

bool UnitTest::multiplyTest( void )
{
	//****** Calculated with pen  Calculated with pen   Calculated with pen
	Vector4 target( 0.0f, 10.0f, 1.0f , 1.0f );
	
	// Testing
	Vector4 result( 1.0f, 1.0f, 0.0f , 1.0f );

	Matrix Translation;
	Translation.Translate(0.0f,9.0f, 0.0f );

	Matrix Rotation;
	Rotation.CreateRotateY((float)(DEG_TO_RADIANS(-90)) ); 


	Matrix final = Rotation *  Translation ;


	result =  result * final;

	if (target == result)
	{
		return true;//multiplyTest
	}
	return false;

}
/*
bool UnitTest::compareResults(bool expect, bool result,const char * name)
{
	if(expect == result)
	{
		cout<<name<<"  passed!"<<"\n"<<endl;
		return true;
	}
	cout<<name<<"  failed!"<<"\n"<<endl;
	return false;

}
bool UnitTest::IsNAN_float(const float i)
{
	cout<<"input:"<<i<<",  ";
	if (!IsNAN(i))
		return true;
	

	return false;
}

bool UnitTest::equals_v1_v2(const Vector3 &vTester,const Vector3 &vTester2)
{

	cout<<"input1:";
	vTester.printOut();
	cout<<"input2:";
	vTester2.printOut();
	
	if (vTester2 == vTester)
	{
		return true;
	}

	

	return false;
}

UnitTest::~UnitTest(void)
{

}

bool UnitTest::ringBufferTest( void )
{
	
	const unsigned int UTRingBufferSize = 12;


	RingBuffer<unsigned int, UTRingBufferSize>	UTRingBuffer;


	assert( UTRingBuffer.getSize() == UTRingBufferSize );
	assert( UTRingBuffer.getCount() == 0 );


	for( unsigned int i = 0; i < (UTRingBufferSize * 2.5); i++ )
	{
		UTRingBuffer.push( i );


		for( unsigned int j = 0; j < UTRingBuffer.getCount(); j++ )
		{
			assert( UTRingBuffer[j] == (i - j) );
		}
	}


	UTRingBuffer.clear();
	assert( UTRingBuffer.getCount() == 0 );
	return true;
}
*/
}