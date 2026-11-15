#pragma once


namespace MyEngine{

	bool IsNAN(const float i_f);

	void static setMaxRelativeError(const float n);

	bool AlmostEqualRelative(const float A,const float B);

}