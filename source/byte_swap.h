#include "stdint.h"

#define bswap32u(x) __builtin_bswap32(x)
#define bswap64u(x) __builtin_bswap64(x)

float bswap32f(const float x);
float bswap64f(const float x);