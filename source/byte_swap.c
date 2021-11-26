#include "byte_swap.h"

#include "stdint.h"

float bswap32f(const float x) {
    union {
        float floatValue;
        uint32_t byteValue;
    } swapper;

    swapper.floatValue = x;
    swapper.byteValue = __builtin_bswap32(swapper.byteValue);

    return swapper.floatValue;
}

double bswap64f(const double x) {
    union {
        double doubleValue;
        uint64_t byteValue;
    } swapper;

    swapper.doubleValue = x;
    swapper.byteValue = __builtin_bswap64(swapper.byteValue);

    return swapper.doubleValue;
}