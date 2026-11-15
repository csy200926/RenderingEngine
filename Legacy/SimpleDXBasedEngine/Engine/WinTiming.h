#pragma once


namespace Timing
{

	class WinTiming
	{

		
	public:
	    static double getCurenntFrameTime_ms();

		static double getLastFrameTime_ms();

		static void setLastFrameTime_ms(double);
	

	};
	
}