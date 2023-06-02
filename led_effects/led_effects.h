#ifndef _LED_EFFECTS_H_
#define _LED_EFFECTS_H_
#include "pico/stdlib.h"
#define COLORS 32

struct rgb
{
    uint8_t r, g, b;
};

struct rgb rgb_init(const uint8_t r, const uint8_t g, const uint8_t b);
struct rgb rgbw_set_brightness(const uint8_t brightness, const struct rgb s);

enum pattern
{
    DISABLED,
    ENABLED,
    BREATHING,
    FILL,
    FILL_TWO_SIDED,
    FADE,
    FADE_MOVING,
    SNAKES,
    SNAKES_FADED,
    SPARKLE,
};
struct pattern_data
{
    uint32_t padding;
    uint32_t max;
};
uint8_t get_pattern(const enum pattern pattern, const uint32_t length, const uint32_t t, const uint32_t i, const struct pattern_data *data);

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
    COLOR_PALETTE,
    COLOR_PALETTE_MOVING,
    COLOR_WIPE,
};
struct color_data
{
    struct rgb colors[COLORS];
    uint8_t selected;
    uint8_t used;
    uint32_t max;
};
struct rgb get_color(const enum color color, const uint32_t t, const uint32_t i, const struct color_data *data);
void generate_color_palette(const uint8_t amount, const struct rgb mix, struct color_data *data);

#endif
