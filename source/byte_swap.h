#define bswap16u(x) __builtin_bswap16(x)
#define bswap32u(x) __builtin_bswap32(x)
#define bswap64u(x) __builtin_bswap64(x)

float bswap32f(const float x);
double bswap64f(const double x);