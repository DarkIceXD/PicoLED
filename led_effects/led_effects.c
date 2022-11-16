#include "led_effects.h"
#include "../algorithms/algorithms.h"
#include <stdlib.h>

#define STATES 64

struct state
{
    uint32_t j;
    uint8_t brightness;
};

static struct state state[STATES];

int rng(const int from, const int to)
{
    return (rand() % (to - from + 1)) + from;
}

struct rgbw rgbw_init(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t w)
{
    return (struct rgbw){.r = r, .g = g, .b = b, .w = w};
}

struct rgbw rgb_init(const uint8_t r, const uint8_t g, const uint8_t b)
{
    return rgbw_init(r, g, b, 0);
}

static inline int scale(const uint8_t a, const uint8_t b, const int32_t s, const int32_t max)
{
    return a + (b - a) * s / max;
}

static struct rgbw gradient(const struct rgbw a, const struct rgbw b, const uint32_t x, const uint32_t max)
{
    return rgbw_init(
        scale(a.r, b.r, x, max),
        scale(a.g, b.g, x, max),
        scale(a.b, b.b, x, max),
        scale(a.w, b.w, x, max));
}

struct rgbw rgbw_set_brightness(const uint8_t brightness, const struct rgbw s)
{
    return gradient(rgbw_init(0, 0, 0, 0), s, brightness, 0xff);
}

static struct rgbw gradient_multi_color(const uint32_t x, const struct color_data *data)
{
    const uint32_t segment = data->max / (data->used - 1);
    const uint32_t normalized = x % data->max;
    const uint32_t state = normalized / segment;
    const uint32_t rem = normalized % segment;
    return gradient(data->colors[state], data->colors[state + 1], rem, segment);
}

static struct rgbw rainbow(const uint32_t x, const uint32_t max)
{
    const uint32_t step = x * (0xff * 3) / max;
    const uint32_t normalized = step % (0xff * 3);
    const uint8_t state = normalized / 0xff;
    const uint8_t rem = normalized % 0xff;
    if (state == 0)
        return rgb_init(0xff - rem, rem, 0);
    else if (state == 1)
        return rgb_init(0, 0xff - rem, rem);
    else
        return rgb_init(rem, 0, 0xff - rem);
}

static struct rgbw color_palette(const uint32_t x, const struct color_data *data)
{
    const uint32_t segment = data->max / data->used;
    const uint32_t normalized = x % data->max;
    const uint32_t state = normalized / segment;
    return data->colors[state];
}

struct rgbw get_color(const enum color color, const uint32_t i, const uint32_t j, const struct color_data *data)
{
    switch (color)
    {
    case RANDOM:
        return rgbw_init(rng(0, 255), rng(0, 255), rng(0, 255), rng(0, 255));
    case GRADIENT:
        return gradient_multi_color(j, data);
    case GRADIENT_MOVING:
        return gradient_multi_color((i + j) % data->max, data);
    case GRADIENT_BREATHING:
        return gradient_multi_color(i % data->max, data);
    case RAINBOW:
        return rainbow(j, data->max);
    case RAINBOW_MOVING:
        return rainbow((i + j) % data->max, data->max);
    case RAINBOW_BREATHING:
        return rainbow(i % data->max, data->max);
    case COLOR_PALETTE:
        return color_palette(j, data);
    case COLOR_PALETTE_MOVING:
        return color_palette((i + j) % data->max, data);
    default:
        return data->colors[min(COLORS - 1, data->selected)];
    }
}

static uint8_t breathing(const uint32_t i)
{
    const uint32_t normalized = i % ((0xff + 1) * 2);
    const uint32_t state = normalized / (0xff + 1);
    const uint32_t rem = normalized % (0xff + 1);
    return state == 0 ? rem : 0xff - rem;
}

static uint8_t fill(const uint32_t i, const uint32_t j, const uint32_t len)
{
    const uint32_t normalized = i % (len * 2);
    const uint32_t state = normalized / len;
    const uint32_t rem = normalized % len;
    if (state == 0)
    {
        if (j < rem)
            return 0xff;
        return 0;
    }
    else
    {
        if (j < len - rem)
            return 0xff;
        return 0;
    }
}

static uint8_t fill_two_sided(const uint32_t i, const uint32_t j, const uint32_t len)
{
    const uint32_t half_len = len / 2;
    const uint32_t normalized = i % len; // (half_len * 2)
    const uint32_t state = normalized / half_len;
    const uint32_t rem = normalized % half_len;
    if (state == 0)
    {
        if (j < rem || j > len - rem)
            return 0xff;
        return 0;
    }
    else
    {
        if (j < half_len - rem || j > half_len + rem)
            return 0xff;
        return 0;
    }
}

static uint8_t fade(const uint32_t j, const uint32_t len)
{
    const uint32_t step = j * (0xff + 1) / len;
    return step % (0xff + 1);
}

static uint8_t snakes(const uint32_t i, const uint32_t j, const struct pattern_data *data)
{
    return (((i + j) % (data->max + data->padding)) <= data->max) ? 0xff : 0;
}

static uint8_t comets(const uint32_t i, const uint32_t j, const struct pattern_data *data)
{
    const uint32_t comet = (i + j) % (data->max + data->padding);
    if (comet > data->max)
        return 0;
    if (comet == 0)
        return 255;
    const uint8_t brightness = fade(data->max - comet, data->max + 1);
    return rng(brightness / 4, brightness);
}

static uint8_t sparkle(const uint32_t option, const uint32_t i, const uint32_t j, const uint32_t len)
{
    if (j == 0)
    {
        for (uint32_t x = 0; x < min(STATES, option); x++)
        {
            if (state[x].brightness == 0)
            {
                if (!(rand() % 16))
                {
                    state[x].j = rand() % len;
                    state[x].brightness = rng(0xff / 3, 0xff);
                }
            }
            else
            {
                state[x].brightness--;
            }
        }
    }

    for (uint32_t x = 0; x < min(STATES, option); x++)
    {
        if (state[x].j == j)
            return state[x].brightness;
    }

    return 0;
}

uint8_t get_pattern(const enum pattern pattern, const uint32_t i, const uint32_t j, const struct pattern_data *data)
{
    switch (pattern)
    {
    case ENABLED:
        return 0xff;
    case BREATHING:
        return breathing(i);
    case FILL:
        return fill(i, j, data->length);
    case FILL_TWO_SIDED:
        return fill_two_sided(i, j, data->length);
    case FADE:
        return fade(j, data->length);
    case FADE_MOVING:
        return fade((i + j) % data->length, data->length);
    case SNAKES:
        return snakes(i, j, data);
    case COMETS:
        return comets(i, j, data);
    case SPARKLE:
        return sparkle(data->max, i, j, data->length);
    default:
        return 0;
    }
}