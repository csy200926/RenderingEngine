#pragma once

#include <map>
#include <string>

#if defined( ENABLE_PROFILING )

class Profiling
{

	struct Accumulator
	{
		double m_Count;
		double m_Sum;
		double m_Max;
		double m_Min;

		Accumulator()
		{
			m_Count = 0;
			m_Sum = 0;
			m_Max = 0;
			m_Min = UINT_MAX;
		}
	} ;

	std::map<std::string, struct Accumulator> s_Accumulators;

public:
	Profiling(void);
	~Profiling(void);


	void addTiming( const char * i_pName, double i_MS );

	void PrintAccumulators( void );
};
extern Profiling g_Profiler;


#define PROFILE_SCOPE_BEGIN(str) { ScopedTimer ( str );
#define PROFILE_SCOPE_END }

#define PROFILE_UNSCOPED(str) ScopedTimer ( str );
#define PROFILE_PRINT_RESULTS g_Profiler.PrintAccumulators();
#else
#define PROFILE_SCOPE_BEGIN(str) __noop
#define PROFILE_SCOPE_END __noop
#define PROFILE_UNSCOPED(str) __noop
#define PROFILE_PRINT_RESULTS __noop;
#endif // ENABLE_PROFILING