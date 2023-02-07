#include "led_effects.h"
#include "../algorithms/algorithms.h"
#include <stdlib.h>

#define STATES 64

struct state
{
    uint32_t i;
    uint8_t brightness;
};

static struct state state[STATES];

static int rng(const int from, const int to)
{
    return (rand() % (to - from + 1)) + from;
}

struct rgb rgb_init(const uint8_t r, const uint8_t g, const uint8_t b)
{
    return (struct rgb){.r = r, .g = g, .b = b};
}

static inline int scale(const uint8_t a, const uint8_t b, const int32_t s, const int32_t max)
{
    return a + (b - a) * s / max;
}

static struct rgb gradient(const struct rgb a, const struct rgb b, const uint32_t x, const uint32_t max)
{
    return rgb_init(
        scale(a.r, b.r, x, max),
        scale(a.g, b.g, x, max),
        scale(a.b, b.b, x, max));
}

struct rgb rgbw_set_brightness(const uint8_t brightness, const struct rgb s)
{
    return gradient(rgb_init(0, 0, 0), s, brightness, 0xff);
}

static uint8_t breathing(const uint32_t t)
{
    const uint32_t normalized = t % ((0xff + 1) * 2);
    const uint32_t state = normalized / (0xff + 1);
    const uint32_t rem = normalized % (0xff + 1);
    return state == 0 ? rem : 0xff - rem;
}

static uint8_t fill(const uint32_t t, const uint32_t i, const uint32_t len)
{
    const uint32_t normalized = t % (len * 2);
    const uint32_t state = normalized / len;
    const uint32_t rem = normalized % len;
    if (state == 0)
    {
        if (i < rem)
            return 0xff;
        return 0;
    }
    else
    {
        if (i < len - rem)
            return 0xff;
        return 0;
    }
}

static uint8_t fill_two_sided(const uint32_t t, const uint32_t i, const uint32_t len)
{
    const uint32_t half_len = len / 2;
    const uint32_t normalized = t % len; // (half_len * 2)
    const uint32_t state = normalized / half_len;
    const uint32_t rem = normalized % half_len;
    if (state == 0)
    {
        if (i < rem || i > len - rem)
            return 0xff;
        return 0;
    }
    else
    {
        if (i < half_len - rem || i > half_len + rem)
            return 0xff;
        return 0;
    }
}

static uint8_t fade(const uint32_t i, const uint32_t len)
{
    const uint32_t step = i * (0xff + 1) / len;
    return step % (0xff + 1);
}

static uint8_t snakes(const uint32_t t, const uint32_t i, const struct pattern_data *data)
{
    return (((t + i) % (data->max + data->padding)) < data->max) ? 0xff : 0;
}

static uint8_t snakes_faded(const uint32_t t, const uint32_t i, const struct pattern_data *data)
{
    const uint32_t faded_snake = (t + i) % (data->max + data->padding);
    if (faded_snake < data->max)
        return fade(data->max - faded_snake, data->max + 1);
    return 0;
}

static uint8_t sparkle(const uint32_t t, const uint32_t i, const uint32_t len, const struct pattern_data *data)
{
    const uint32_t state_length = min(STATES, data->max);
    if (i == 0)
    {
        for (uint32_t x = 0; x < state_length; x++)
        {
            if (state[x].brightness == 0)
            {
                if (!(rand() % data->padding))
                {
                    state[x].i = rand() % len;
                    state[x].brightness = rng(0xff / 3, 0xff);
                }
            }
            else
            {
                state[x].brightness--;
            }
        }
    }

    for (uint32_t x = 0; x < state_length; x++)
    {
        if (state[x].i == i)
            return state[x].brightness;
    }

    return 0;
}

uint8_t get_pattern(const enum pattern pattern, const uint32_t length, const uint32_t t, const uint32_t i, const struct pattern_data *data)
{
    switch (pattern)
    {
    case ENABLED:
        return 0xff;
    case BREATHING:
        return breathing(t);
    case FILL:
        return fill(t, i, length);
    case FILL_TWO_SIDED:
        return fill_two_sided(t, i, length);
    case FADE:
        return fade(i, length);
    case FADE_MOVING:
        return fade((t + i) % length, length);
    case SNAKES:
        return snakes(t, i, data);
    case SNAKES_FADED:
        return snakes_faded(t, i, data);
    case SPARKLE:
        return sparkle(t, i, length, data);
    default:
        return 0;
    }
}

static struct rgb gradient_multi_color(const uint32_t x, const struct color_data *data)
{
    const uint32_t segment = data->max / (data->used - 1);
    const uint32_t normalized = x % data->max;
    const uint32_t state = normalized / segment;
    const uint32_t rem = normalized % segment;
    return gradient(data->colors[state], data->colors[state + 1], rem, segment);
}

static struct rgb rainbow(const uint32_t x, const uint32_t max)
{
    const uint32_t step = (x % max) * (0xff * 3) / max;
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

static struct rgb color_palette(const uint32_t x, const struct color_data *data)
{
    const uint32_t segment = data->max / data->used;
    const uint32_t normalized = x % data->max;
    const uint32_t state = normalized / segment;
    return data->colors[state];
}

static struct rgb color_wipe(const uint32_t t, const uint32_t i, const struct color_data *data)
{
    const uint32_t state = t / data->max;
    const uint32_t rem = t % data->max;
    if ((i % data->max) < rem)
        return data->colors[(state + 1) % data->used];
    return data->colors[state % data->used];
}

struct rgb get_color(const enum color color, const uint32_t t, const uint32_t i, const struct color_data *data)
{
    switch (color)
    {
    case RANDOM:
        return rgb_init(rng(0, 255), rng(0, 255), rng(0, 255));
    case GRADIENT:
        return gradient_multi_color(i, data);
    case GRADIENT_MOVING:
        return gradient_multi_color(t + i, data);
    case GRADIENT_BREATHING:
        return gradient_multi_color(t, data);
    case RAINBOW:
        return rainbow(i, data->max);
    case RAINBOW_MOVING:
        return rainbow(t + i, data->max);
    case RAINBOW_BREATHING:
        return rainbow(t, data->max);
    case COLOR_PALETTE:
        return color_palette(i, data);
    case COLOR_PALETTE_MOVING:
        return color_palette(t + i, data);
    case COLOR_WIPE:
        return color_wipe(t, i, data);
    default:
        return data->colors[min(COLORS - 1, data->selected)];
    }
}