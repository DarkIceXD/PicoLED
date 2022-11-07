#ifndef _LED_EFFECTS_H_
#define _LED_EFFECTS_H_
#include "pico/stdlib.h"
#define COLORS 8

struct rgbw
{
    uint8_t r, g, b, w;
};

struct rgbw rgbw_init(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t w);
struct rgbw rgb_init(const uint8_t r, const uint8_t g, const uint8_t b);
struct rgbw rgbw_set_brightness(const uint8_t brightness, const struct rgbw s);

enum color
{
    SELECTED,
    RANDOM,
    GRADIENT,
    GRADIENT_MOVING,
    GRADIENT_BREATHING,
    RAINBOW,
    RAINBOW_MOVING,
    RAINBOW_BREATHING,
};
struct color_data
{
    struct rgbw colors[COLORS];
    uint8_t selected;
    uint8_t used;
    uint32_t max;
};
struct rgbw get_color(const enum color color, const uint32_t i, const uint32_t j, const struct color_data *data);

enum pattern
{
    DISABLED,
    ENABLED,
    SPARKLE,
    SNAKES,
    BREATHING,
    FILL,
    FILL_TWO_SIDED,
    FADE,
    FADE_MOVING,
};
struct pattern_data
{
    uint32_t padding;
    uint32_t max;
    uint32_t length;
};
uint8_t get_pattern(const enum pattern pattern, const uint32_t i, const uint32_t j, const struct pattern_data *data);

#endif
