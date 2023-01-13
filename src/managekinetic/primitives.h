#ifndef MANAGE_KINETIC_PRIMITIVES_H
#define MANAGE_KINETIC_PRIMITIVES_H
#include <cstdint>

typedef uint8_t 		ui8;
typedef uint16_t 		ui16;
typedef uint32_t 		ui32;
typedef uint64_t 		ui64;
typedef int8_t 			i8;
typedef int16_t 		i16;
typedef int32_t 		i32;
typedef int64_t 		i64;

typedef float 			r32;
typedef double 			r64;

typedef int32_t 		b32;
typedef int64_t 		b64;

#define KILOBYTES(n) (ui64)(n*(ui64)1024)
#define MEGABYTES(n) (ui64)(KILOBYTES(n)*(ui64)n)
#define GIGABYTES(n) (ui64)(MEGABYTES(n)*(ui64)n)
#define TERABYTES(n) (ui64)(TERABYTES(n)*(ui64)n)

#endif