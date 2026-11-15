#include "Profiling.h"


#if defined( ENABLE_PROFILING )
Profiling g_Profiler;


Profiling::Profiling(void)
{
}


Profiling::~Profiling(void)
{
}

void Profiling::addTiming( const char * i_pName, double i_MS )
{

	struct Accumulator & myAccumulator = s_Accumulators[i_pName];

	if( i_MS < myAccumulator.m_Min )
	myAccumulator.m_Min = i_MS;
	if( i_MS > myAccumulator.m_Max )
	myAccumulator.m_Max = i_MS;

	myAccumulator.m_Count++;
	myAccumulator.m_Sum += i_MS;

}

void Profiling::PrintAccumulators( void )
{
	std::map<std::string, struct Accumulator>::iterator iter = s_Accumulators.begin();

	for( iter ; iter != s_Accumulators.end(); ++iter )
	{
		double Average = iter->second.m_Count ? ((double) iter->second.m_Sum ) / iter->second.m_Count : 0.0f;

		printf( "[%s] Count: %f Sum: %f Min: %f Max: %f Ave: %f\n", iter->first.c_str(), iter->second.m_Count, iter->second.m_Sum, iter->second.m_Min, iter->second.m_Max, Average );
	}
} 

#endif // ENABLE_PROFILING