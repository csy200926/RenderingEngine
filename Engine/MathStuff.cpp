#include "MathStuff.h"
#include "math.h"

namespace MyEngine
{

	static float maxRelativeError = 0.001f;

	void setMaxRelativeError(const float n)
	{
		maxRelativeError = fabsf(n);
	}

	bool IsNAN(const float i_f)
	{

		volatile float i = i_f;

		return i != i;
	}

	// Reference: http://www.cygnus-software.com/papers/comparingfloats/comparingfloats.htm
	bool AlmostEqualRelative(const float m, const float n)
	{
		if (m == n)
			return true;
		float relativeError = fabs(m - n);
		if (relativeError <= maxRelativeError)
			return true;
		return false;
	}

} // namespace MyEngine
