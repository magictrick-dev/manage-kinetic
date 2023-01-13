#ifndef MANAGE_KINETIC_BENCHMARK_WIN32_H
#define MANAGE_KINETIC_BENCHMARK_WIN32_H
#include <managekinetic/primitives.h>
#include <string>

#define SECONDSTOMILISECONDS 	(1000)
#define SECONDSTOMICROSECONDS 	(1000 * SECONDSTOMILISECONDS)

ui64 benchmarkCreateTimestamp();
r64 benchmarkMilisecondsElapsed(ui64 then, ui64 now);
r64 benchmarkSecondsElapsed(ui64 then, ui64 now);
std::string benchmarkString(r64 benchmarkTime);
std::string benchmarkNowMilliseconds(ui64 then);
std::string benchmarkNowSeconds(ui64 then);

#endif