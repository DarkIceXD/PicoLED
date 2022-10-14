#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/cyw43_arch.h"

#include "dhcpserver/dhcpserver.h"
#include "dnsserver/dnsserver.h"
#include "httpserver/httpserver.h"
#include "led_effects/led_effects.h"

#include "ws2812.pio.h"
#define TEMP_SIZE 16

struct settings
{
    uint32_t led_count;
    uint32_t delay;
    uint8_t brightness;
    enum color c;
    struct rgbw primary;
    struct rgbw secondary;
    uint32_t max_value;
    enum pattern p;
    uint32_t option;
} settings;

static const char *http_post_handler(const char *path, const char *content)
{
    if (strncmp(path, "/api/get", 8) == 0)
    {
        static char response[TEMP_SIZE];
        switch (atoi(content))
        {
        case 0:
            snprintf(response, TEMP_SIZE, "%lu", settings.led_count);
            break;
        case 1:
            snprintf(response, TEMP_SIZE, "%lu", settings.delay);
            break;
        case 2:
            snprintf(response, TEMP_SIZE, "%u", settings.brightness);
            break;
        case 3:
            snprintf(response, TEMP_SIZE, "%u", settings.c);
            break;
        case 4:
            snprintf(response, TEMP_SIZE, "#%2x%2x%2x", settings.primary.r, settings.primary.g, settings.primary.b);
            break;
        case 5:
            snprintf(response, TEMP_SIZE, "#%2x%2x%2x", settings.secondary.r, settings.secondary.g, settings.secondary.b);
            break;
        case 6:
            snprintf(response, TEMP_SIZE, "%lu", settings.max_value);
            break;
        case 7:
            snprintf(response, TEMP_SIZE, "%u", settings.p);
            break;
        case 8:
            snprintf(response, TEMP_SIZE, "%lu", settings.option);
            break;
        default:
            return NULL;
        }
        return response;
    }
    else
    {
        char *first_end = strchr(content, ',');
        if (!first_end)
            return 0;
        first_end += 1;
        switch (atoi(content))
        {
        case 0:
            settings.led_count = atoi(first_end);
            break;
        case 1:
            settings.delay = atoi(first_end);
            break;
        case 2:
            settings.brightness = atoi(first_end);
            break;
        case 3:
            settings.c = atoi(first_end);
            break;
        case 4:
        {
            int r, g, b;
            sscanf(first_end, "#%2x%2x%2x", &r, &g, &b);
            settings.primary = rgb_init(r, g, b);
            break;
        }
        case 5:
        {
            int r, g, b;
            sscanf(first_end, "#%2x%2x%2x", &r, &g, &b);
            settings.secondary = rgb_init(r, g, b);
            break;
        }
        case 6:
            settings.max_value = atoi(first_end);
            break;
        case 7:
            settings.p = atoi(first_end);
            break;
        case 8:
            settings.option = atoi(first_end);
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

static inline uint32_t grbw(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t w)
{
    return ((uint32_t)(r) << 16) | ((uint32_t)(g) << 24) | ((uint32_t)(b) << 8) | ((uint32_t)(w) << 0);
}

static inline uint32_t rgbw_to_grbw(const struct rgbw s)
{
    return grbw(s.r, s.g, s.b, s.w);
}

void core1_entry()
{
    const PIO pio = pio0;
    const uint offset = pio_add_program(pio, &ws2812_program);
    const int sm = 0;
    const int gpio_pin = 2;
    const bool is_rgbw = false;
    ws2812_program_init(pio, sm, offset, gpio_pin, 800000, is_rgbw);
    settings.led_count = 300;
    settings.delay = 25;
    settings.brightness = 100;
    settings.c = LINEAR_GRADIENT;
    settings.primary = rgb_init(0, 0, 0);
    settings.secondary = rgb_init(255, 255, 255);
    settings.max_value = settings.led_count;
    settings.p = FILL_TWO_SIDED;
    settings.option = 20;
    for (uint32_t i = 0;; ++i)
    {
        for (uint32_t j = 0; j < settings.led_count; j++)
        {
            const uint8_t pattern = get_pattern(settings.p, settings.option, i, j, settings.led_count);
            if (pattern)
                put_pixel(rgbw_to_grbw(rgbw_set_brightness(settings.brightness, rgbw_set_brightness(pattern, get_color(settings.c, settings.primary, settings.secondary, i, j, settings.max_value)))));
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

    static const char *ap_name = "DarkLEDs";
    static const char *password = "password";
    cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK);

    ip4_addr_t gw, mask;
    IP4_ADDR(&gw, 192, 168, 4, 1);
    IP4_ADDR(&mask, 255, 255, 255, 0);

    // Start the dhcp server
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
