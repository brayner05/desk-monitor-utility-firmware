#include <stdio.h>
#include "main.h"

static const uint8_t LCD_WIDTH = 16;

static void pretty_print_temperature(const float temp_celsius) {
    int digits = 1;
    if (temp_celsius >= 100.0)
        digits = 3;
    else if (temp_celsius >= 10.0)
        digits = 2;

    // Up to 3 digits + the degrees symbol + C + temperature glyph:
    const int start_column = LCD_WIDTH - (digits + 3);
    display_goto(start_column - 1, 0);
    putchar(' ');
    printf("%c%d%cC", GLYPH_TEMPERATURE, (int) temp_celsius, (char) 223);
}

static void pretty_print_lighting(const float light_level) {
    int digits = 1;
    if (light_level >= 100.0)
        digits = 3;
    else if (light_level >= 10.0)
        digits = 2;

    // Up to 3 digits + % + temperature glyph:
    const int start_column = LCD_WIDTH - (digits + 2);
    display_goto(start_column - 1, 1);
    putchar(' ');
    printf("%c%d%%", GLYPH_BRIGHTNESS, (int) light_level);
}

extern void app_main(void) {
    display_goto(0, 0);
    putchar(GLYPH_WIFI);
    display_goto(0, 1);
    printf("00:00");
    while (1) {
        if (tmp36_data_ready()) {
            const uint16_t tmp36_reading = tmp36_read();
            const float tmp36_pin_voltage = tmp36_reading * 5.0 / 1023.0;
            const float temperature_celsius = (tmp36_pin_voltage - 0.5) * 100.0;
            pretty_print_temperature(temperature_celsius);
        }
        if (photoresistor_data_ready()) {
            const uint16_t photoresistor_reading = photoresistor_read();
            const float light_level = 100 * photoresistor_reading / 1023.0;
            pretty_print_lighting(light_level);
        }
    }
}
