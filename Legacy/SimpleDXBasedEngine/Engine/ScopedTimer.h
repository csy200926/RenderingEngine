#pragma once
#include "Extension.h"
#include "WinTiming.h"
#include "Profiling.h"
#if defined( ENABLE_PROFILING )
class ScopedTimer
{
	const char * m_pScopeName;
	double  m_Start;
	
	public:
	ScopedTimer( const char * i_pName ) :
		m_Start( Timing::WinTiming::getCurenntFrameTime_ms() ),
	m_pScopeName( i_pName )
	{
		
		printf( "Scoped Timer %s started at  %f\n", m_pScopeName ? m_pScopeName : "Unnamed", m_Start );

	}

	~ScopedTimer()
	{
		double end = Timing::WinTiming::getCurenntFrameTime_ms();

		double difference = end - m_Start;

		printf( "Scoped Timer %s ended at %f ran for %f seconds\n", m_pScopeName ? m_pScopeName : "Unnamed", end, difference );
	
		g_Profiler.addTiming(m_pScopeName, difference);

	}

};
#endif // ENABLE_PROFILING