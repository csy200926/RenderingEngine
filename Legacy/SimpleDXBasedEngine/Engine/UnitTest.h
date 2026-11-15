#pragma once
#include "Vector3.h"
namespace MyEngine{
	class UnitTest
	{
	public:


		UnitTest(void);
		~UnitTest(void);

		bool runTest();
		/*
		bool compareResults(bool expect, bool result,const char * name);

		bool IsNAN_float(const float i);
		bool equals_v1_v2(const Vector3 &vTester,const Vector3 &vTester2);

		bool ringBufferTest();

		*/
		bool IdentityTest(void);
		bool TransposeTest(void);
		bool InverseTest(void);

		bool CreateRotateTest( void );
		bool TranslateTest( void );

		bool multiplyTest( void );


	};

}