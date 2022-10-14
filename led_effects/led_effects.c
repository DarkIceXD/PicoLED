#include "led_effects.h"
#include <stdlib.h>

struct rgbw rgbw_init(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t w)
{
    return (struct rgbw){.r = r, .g = g, .b = b, .w = w};
}

struct rgbw rgb_init(const uint8_t r, const uint8_t g, const uint8_t b)
{
    return rgbw_init(r, g, b, 0);
}

struct rgbw rgbw_set_brightness(const uint8_t brightness, const struct rgbw s)
{
    return rgbw_init(
        s.r * brightness / 0xff,
        s.g * brightness / 0xff,
        s.b * brightness / 0xff,
        s.w * brightness / 0xff);
}

static struct rgbw linear_gradient(const struct rgbw primary, const struct rgbw secondary, const uint32_t x, const uint32_t max)
{
    return rgbw_init(
        primary.r + (secondary.r - primary.r) * x / max,
        primary.g + (secondary.g - primary.g) * x / max,
        primary.b + (secondary.b - primary.b) * x / max,
        primary.w + (secondary.w - primary.w) * x / max);
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

struct rgbw get_color(const enum color c, const struct rgbw primary, const struct rgbw secondary, const uint32_t i, const uint32_t j, const uint32_t max)
{
    switch (c)
    {
    case SECONDARY:
        return secondary;
    case RANDOM:
        return rgb_init(rand(), rand(), rand());
    case LINEAR_GRADIENT:
        return linear_gradient(primary, secondary, j, max);
    case LINEAR_GRADIENT_MOVING:
        return linear_gradient(primary, secondary, (i + j) % max, max);
    case RAINBOW:
        return rainbow(j, max);
    case RAINBOW_MOVING:
        return rainbow((i + j) % max, max);
    default:
        return primary;
    }
}

static uint8_t sparkle(const uint32_t option, const uint32_t i, const uint32_t j, const uint32_t len)
{
    const uint32_t normalized = i % (0xff * len / option);
    const uint8_t state = normalized / 0xff;
    if ((j % option) == state)
        return 0xff - (normalized % 0xff);

    return 0;
}

static uint8_t snake(const uint32_t option, const uint32_t i, const uint32_t j, const uint32_t len)
{
    const uint32_t x = i % len;
    return ((x - option) < j && j <= x) ? 0xff : 0;
}

static uint8_t breathing(const uint32_t i)
{
    const uint32_t normalized = i % (0xff * 2);
    const uint32_t state = normalized / 0xff;
    const uint32_t rem = normalized % 0xff;
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

uint8_t get_pattern(const enum pattern p, const uint32_t option, const uint32_t i, const uint32_t j, const uint32_t len)
{
    switch (p)
    {
    case ENABLED:
        return 0xff;
    case SPARKLE:
        return sparkle(option, i, j, len);
    case SNAKE:
        return snake(option, i, j, len);
    case MULTIPLE_SNAKES:
        return (((i + j) % option) < option / 3) ? 0xff : 0;
    case BREATHING:
        return breathing(i);
    case FILL:
        return fill(i, j, len);
    case FILL_TWO_SIDED:
        return fill_two_sided(i, j, len);
    default:
        return 0;
    }
}