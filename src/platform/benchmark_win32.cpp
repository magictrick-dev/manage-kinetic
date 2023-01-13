#include <platform/benchmark_win32.h>
#include <windows.h>
#include <sstream>
#include <iomanip>

static inline ui64
getPerformanceFrequency()
{

	static LARGE_INTEGER s_frequency = {};
	static bool s_isQueried = false;

	// Cache the query, we only need to do this once.
	if (s_isQueried == false)
	{
		QueryPerformanceFrequency(&s_frequency);
		s_isQueried = true;
	}
	
	return (ui64)s_frequency.QuadPart;

}

ui64
benchmarkCreateTimestamp()
{

	LARGE_INTEGER time = {};
	QueryPerformanceCounter(&time);
	return (ui64)time.QuadPart;

}

r64
benchmarkMilisecondsElapsed(ui64 then, ui64 now)
{

	ui64 ticksElapsed = now - then;
	ui64 milisecondsFromTicks = ticksElapsed * SECONDSTOMILISECONDS;
	r64 benchmarkedTime = (r64)milisecondsFromTicks / (r64)getPerformanceFrequency();
	return benchmarkedTime;

}

r64
benchmarkSecondsElapsed(ui64 then, ui64 now)
{

	ui64 ticksElapsed = now - then;
	ui64 milisecondsFromTicks = ticksElapsed;
	r64 benchmarkedTime = (r64)milisecondsFromTicks / (r64)getPerformanceFrequency();
	return benchmarkedTime;

}

std::string
benchmarkString(r64 benchmarkTime)
{
	std::stringstream ss = {};
	ss << std::fixed << std::setprecision(2) << benchmarkTime;
	return ss.str();

}

std::string
benchmarkNowMilliseconds(ui64 then)
{

	ui64 now = benchmarkCreateTimestamp();
	r64 ms = benchmarkMilisecondsElapsed(then, now);
	return benchmarkString(ms);

}

std::string
benchmarkNowSeconds(ui64 then)
{

	ui64 now = benchmarkCreateTimestamp();
	r64 s = benchmarkSecondsElapsed(then, now);
	return benchmarkString(s);

}
