#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/cyw43_arch.h"

#include "dhcpserver/dhcpserver.h"
#include "dnsserver/dnsserver.h"
#include "httpserver/httpserver.h"
#include "led_effects/led_effects.h"
#include "storage/storage.h"

#include "ws2812.pio.h"
#define TEMP_SIZE 128

struct settings
{
    char ap_name[32 + 1];
    char password[64 + 1];
    uint32_t delay;
    uint32_t length;
    uint8_t brightness;
    uint8_t reverse;
    enum pattern p;
    struct pattern_data pattern;
    enum color c;
    struct color_data color;
};
static struct settings settings = {
    .ap_name = "DarkLEDs",
    .password = "password",
    .delay = 40,
    .length = 300,
    .brightness = 100,
    .reverse = 0,
    .p = SNAKES_FADED,
    .pattern.padding = 20,
    .pattern.max = 30,
    .c = RAINBOW,
    .color.colors[0] = {.r = 255, .g = 255, .b = 255},
    .color.colors[1] = {.r = 255, .g = 255, .b = 255},
    .color.selected = 0,
    .color.used = 2,
    .color.max = 300,
};

static const char *http_post_handler(const char *path, const char *content)
{
    const char delim = ';';
    if (strncmp(path, "/api/get", 8) == 0)
    {
        static char response[TEMP_SIZE];
        switch (atoi(content))
        {
        case 0:
            snprintf(response, TEMP_SIZE, "%lu", settings.length);
            break;
        case 1:
            snprintf(response, TEMP_SIZE, "%lu", settings.delay);
            break;
        case 2:
            snprintf(response, TEMP_SIZE, "%u", settings.brightness);
            break;
        case 3:
            snprintf(response, TEMP_SIZE, "%u", settings.reverse);
            break;
        case 4:
            snprintf(response, TEMP_SIZE, "%u", settings.p);
            break;
        case 5:
            snprintf(response, TEMP_SIZE, "%lu", settings.pattern.padding);
            break;
        case 6:
            snprintf(response, TEMP_SIZE, "%lu", settings.pattern.max);
            break;
        case 7:
            snprintf(response, TEMP_SIZE, "%u", settings.c);
            break;
        case 8:
        {
            char *first_end = strchr(content, delim);
            if (!first_end)
                return 0;
            first_end += 1;
            switch (atoi(first_end))
            {
            case 0:
                snprintf(response, TEMP_SIZE, "%u", settings.color.used);
                break;
            case 1:
                snprintf(response, TEMP_SIZE, "%u", settings.color.selected);
                break;
            case 2:
            {
                first_end = strchr(first_end, delim);
                if (!first_end)
                    return 0;
                first_end += 1;
                const int index = atoi(first_end);
                if (index >= COLORS)
                    return 0;
                snprintf(response, TEMP_SIZE, "#%02x%02x%02x", settings.color.colors[index].r, settings.color.colors[index].g, settings.color.colors[index].b);
                break;
            }
            case 3:
                snprintf(response, TEMP_SIZE, "%u", COLORS);
                break;
            default:
                return NULL;
            }
            break;
        }
        break;
        case 9:
            snprintf(response, TEMP_SIZE, "%lu", settings.color.max);
            break;
        case 10:
            snprintf(response, TEMP_SIZE, "%s", settings.ap_name);
            break;
        case 11:
            snprintf(response, TEMP_SIZE, "%s", settings.password);
            break;
        default:
            return NULL;
        }
        return response;
    }
    else
    {
        char *first_end = strchr(content, delim);
        if (!first_end)
            return 0;
        first_end += 1;
        switch (atoi(content))
        {
        case 0:
            settings.length = atoi(first_end);
            break;
        case 1:
            settings.delay = atoi(first_end);
            break;
        case 2:
            settings.brightness = atoi(first_end);
            break;
        case 3:
            settings.reverse = atoi(first_end);
            break;
        case 4:
            settings.p = atoi(first_end);
            break;
        case 5:
            settings.pattern.padding = atoi(first_end);
            break;
        case 6:
            settings.pattern.max = atoi(first_end);
            break;
        case 7:
            settings.c = atoi(first_end);
            break;
        case 8:
        {
            const int option = atoi(first_end);
            first_end = strchr(first_end, delim);
            if (!first_end)
                return 0;
            first_end += 1;
            switch (option)
            {
            case 0:
            {
                const int new_value = atoi(first_end);
                if (new_value < 2 || new_value >= COLORS)
                    return 0;
                settings.color.used = new_value;
                break;
            }
            case 1:
            {
                const int new_value = atoi(first_end);
                if (new_value < 0 || new_value >= settings.color.used)
                    return 0;
                settings.color.selected = new_value;
                break;
            }
            case 2:
            {
                const int index = atoi(first_end);
                if (index < 0 || index >= COLORS)
                    return 0;
                first_end = strchr(first_end, delim);
                if (!first_end)
                    return 0;
                first_end += 1;
                int r, g, b;
                sscanf(first_end, "#%2x%2x%2x", &r, &g, &b);
                settings.color.colors[index] = rgb_init(r, g, b);
                break;
            }
            default:
                return NULL;
            }
            break;
        }
        case 9:
            settings.color.max = atoi(first_end);
            break;
        case 10:
            strcpy(settings.ap_name, first_end);
            break;
        case 11:
            strcpy(settings.password, first_end);
            break;
        case 12:
            save(&settings, sizeof(settings));
            break;
        default:
            return NULL;
        }
        return "";
    }
}

static inline void put_pixel(const uint32_t pixel)
{
    pio_sm_put_blocking(pio0, 0, pixel);
}

static inline uint32_t grbw(const uint8_t r, const uint8_t g, const uint8_t b)
{
    const uint8_t w = 0;
    return ((uint32_t)(r) << 16) | ((uint32_t)(g) << 24) | ((uint32_t)(b) << 8) | ((uint32_t)(w) << 0);
}

static inline uint32_t rgbw_to_grbw(const struct rgb s)
{
    return grbw(s.r, s.g, s.b);
}

void core1_entry()
{
    const PIO pio = pio0;
    const uint offset = pio_add_program(pio, &ws2812_program);
    const int sm = 0;
    const int gpio_pin = 2;
    const bool is_rgbw = false;
    ws2812_program_init(pio, sm, offset, gpio_pin, 800000, is_rgbw);
    for (uint32_t t = 0;; t++)
    {
        for (uint32_t i = 0; i < settings.length; i++)
        {
            const uint32_t adjusted_i = !settings.reverse ? i : settings.length - 1 - i;
            const uint8_t pattern = get_pattern(settings.p, settings.length, t, adjusted_i, &settings.pattern);
            if (pattern)
                put_pixel(rgbw_to_grbw(rgbw_set_brightness(settings.brightness, rgbw_set_brightness(pattern, get_color(settings.c, t, adjusted_i, &settings.color)))));
            else
                put_pixel(0);
        }
        sleep_ms(settings.delay);
    }
}

int main()
{
    if (cyw43_arch_init())
        return 1;

    load(&settings, sizeof(settings));
    cyw43_arch_enable_ap_mode(settings.ap_name, settings.password, CYW43_AUTH_WPA2_AES_PSK);

    ip4_addr_t gw, mask;
    IP4_ADDR(&gw, 192, 168, 4, 1);
    IP4_ADDR(&mask, 255, 255, 255, 0);

    dhcp_server_t dhcp_server;
    dhcp_server_init(&dhcp_server, &gw, &mask);

    dns_server_t dns_server;
    dns_server_init(&dns_server, &gw);

    http_server_t http_server;
    http_server_init(&http_server, &gw, http_post_handler);

    multicore_launch_core1(core1_entry);
    while (true)
    {
        cyw43_arch_poll();
        sleep_ms(1);
    }
    http_server_deinit(&http_server);
    dns_server_deinit(&dns_server);
    dhcp_server_deinit(&dhcp_server);
    cyw43_arch_deinit();
    return 0;
}
