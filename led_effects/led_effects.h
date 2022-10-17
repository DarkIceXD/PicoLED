#ifndef _LED_EFFECTS_H_
#define _LED_EFFECTS_H_
#include "pico/stdlib.h"

struct rgbw
{
    uint8_t r, g, b, w;
};

struct rgbw rgbw_init(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t w);
struct rgbw rgb_init(const uint8_t r, const uint8_t g, const uint8_t b);
struct rgbw rgbw_set_brightness(const uint8_t brightness, const struct rgbw s);

enum color
{
    PRIMARY,
    SECONDARY,
    RANDOM,
    GRADIENT,
    GRADIENT_MOVING,
    GRADIENT_TWO_SIDED,
    GRADIENT_TWO_SIDED_MOVING,
    GRADIENT_BREATHING,
    RAINBOW,
    RAINBOW_MOVING,
    RAINBOW_BREATHING,
};
struct rgbw get_color(const enum color c, const struct rgbw primary, const struct rgbw secondary, const uint32_t i, const uint32_t j, const uint32_t max);

enum pattern
{
    DISABLED,
    ENABLED,
    SPARKLE,
    RANDOM_SPARKLE,
    SNAKE,
    MULTIPLE_SNAKES,
    BREATHING,
    FILL,
    FILL_TWO_SIDED,
    FADE,
    FADE_MOVING,
};
uint8_t get_pattern(const enum pattern p, const uint32_t option, const uint32_t i, const uint32_t j, const uint32_t len);

#endif
