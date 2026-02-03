#include <stdio.h>
#include "main.h"

static void pretty_print_temperature(const float temp_celsius) {
    const int lcd_width = 16;

    int digits = 1;
    if (temp_celsius >= 100.0)
        digits = 3;
    else if (temp_celsius >= 10.0)
        digits = 2;

    // Up to 3 digits + the degrees symbol + C:
    const int start_column = lcd_width - (digits + 2);
    display_goto(start_column, 0);
    printf("%d%cC", (int) temp_celsius, (char) 223);
}

extern void app_main(void) {
    while (1) {
        if (tmp36_data_ready()) {
            const uint16_t tmp36_reading = tmp36_read();
            const float tmp36_pin_voltage = tmp36_reading * 5.0 / 1023.0;
            const float temperature_celsius = (tmp36_pin_voltage - 0.5) * 100.0;
            pretty_print_temperature(temperature_celsius);
        }
    }
}