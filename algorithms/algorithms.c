#include "algorithms.h"

int min(const int a, const int b)
{
    return a < b ? a : b;
}

int max(const int a, const int b)
{
    return a > b ? a : b;
}

int clamp(const int value, const int minimum, const int maximum)
{
    return min(max(value, minimum), maximum);
}
