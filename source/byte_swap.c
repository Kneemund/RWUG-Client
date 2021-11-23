#include "byte_swap.h"

float bswap32f(const float x) {
    union {
        float floatValue;
        uint32_t byteValue;
    } swapper;

    swapper.floatValue = x;
    swapper.byteValue = __builtin_bswap32(swapper.byteValue);

    return swapper.floatValue;
}

float bswap64f(const float x) {
    union {
        float floatValue;
        uint64_t byteValue;
    } swapper;

    swapper.floatValue = x;
    swapper.byteValue = __builtin_bswap64(swapper.byteValue);

    return swapper.floatValue;
}