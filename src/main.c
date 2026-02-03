#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdbool.h>
#include "pinout.h"
#include "main.h"

static int impl_put_char(char ch, FILE *stream __attribute__((unused)));
static FILE display_stream_out = FDEV_SETUP_STREAM(impl_put_char, NULL, _FDEV_SETUP_WRITE); // NOLINT

#define MODE_DATA ((uint8_t) 1)
#define MODE_CMD  ((uint8_t) 0)
static void display_set_mode(const uint8_t mode) {
    if (mode == MODE_DATA) {
        PORTB |= _BV(PIN_LCD_RS);
    } else {
        PORTB &= ~_BV(PIN_LCD_RS);
    }
}

static void display_enable(void) {
    PORTB |= _BV(PIN_LCD_EN);
    _delay_us(50);
    PORTB &= ~_BV(PIN_LCD_EN);
    _delay_us(50);
}

static void display_send_nibble(const uint8_t nibble) {
    const uint8_t mask = ~(_BV(PIN_LCD_D4) | _BV(PIN_LCD_D5) | _BV(PIN_LCD_D6) | _BV(PIN_LCD_D7));
    PORTB &= mask;
    if (nibble & 0x01) PORTB |= _BV(PIN_LCD_D4);
    if (nibble & 0x02) PORTB |= _BV(PIN_LCD_D5);
    if (nibble & 0x04) PORTB |= _BV(PIN_LCD_D6);
    if (nibble & 0x08) PORTB |= _BV(PIN_LCD_D7);

    display_enable();
}

static void display_send_byte(const uint8_t data, uint8_t mode) {
    display_set_mode(mode);
    display_send_nibble(data >> 4);
    display_send_nibble(data & 0x0F);
}

#define CMD_CLEAR_SCREEN    (1 << 0)
#define CMD_CURSOR_RETURN   (1 << 1)
#define CMD_INPUT_SET       (1 << 2)
#define CMD_DISPLAY_SWITCH  (1 << 3)
#define CMD_SHIFT           (1 << 4)
#define CMD_FUNC_SET        (1 << 5)
#define CMD_CGRAM_AD_SET    (1 << 6)
#define CMD_DDRAM_AD_SET    (1 << 7)
static void display_execute(const uint8_t command) {
    display_send_byte(command, MODE_CMD);

    /*
     * Some commands require a delay to work properly.
     * These delays can be found in section 11 of the datasheet:
     * https://content.arduino.cc/assets/LCDscreen.PDF
     */
    switch (command) {
        case CMD_CLEAR_SCREEN:
        case CMD_CURSOR_RETURN: {
            _delay_ms(2);
            break;
        }
        default: {
            _delay_us(40);
        }
    }
}

extern void display_clear(void) {
    display_execute(0x01);
}

extern void display_goto(const uint8_t col, const uint8_t row) {
    const uint8_t clamped_column = col % 16;
    const uint8_t DDRAM_address = (row == 0 ? 0x00 : 0x40) + clamped_column;
    display_execute(CMD_DDRAM_AD_SET | DDRAM_address);
}

static void display_init(void) {
    _delay_ms(20);
    display_set_mode(MODE_CMD);

    // Startup sequence:
    display_send_nibble(0x03);
    _delay_us(5);
    display_send_nibble(0x03);
    _delay_us(100);
    display_send_nibble(0x03);
    _delay_us(100);

    // 4-bit mode:
    display_send_nibble(0x02);
    _delay_us(100);

    // 4-bits, 2 lines, 5x8 dots:
    display_execute(0x28);

    // Display ON, cursor OFF:
    const uint8_t display_on = (1 << 2);
    const uint8_t cursor_on = (1 << 1);
    display_execute(CMD_DISPLAY_SWITCH | (display_on & ~cursor_on));

    // Entry mode - increment:
    const uint8_t increment_mode = 0x02;
    display_execute(CMD_INPUT_SET | increment_mode);

    // Clear the screen:
    display_clear();

    stdout = &display_stream_out;
}

static int impl_put_char(const char ch, FILE *stream __attribute__((unused))) {
    if (ch == '\n')
        display_goto(0, 1);
    else if (ch == '\r')
        display_execute(CMD_CURSOR_RETURN);
    else
        display_send_byte(ch, MODE_DATA);
    return 0;
}

/**
 * @see Section 23.9 (.1) (Register Description) of ATMega328P datasheet.
 */
static void adc_init(void) {
    ADMUX |= _BV(REFS0);    // Set reference voltage to AVCC (5V).

    /*
     * Set the prescaler value to 128.
     * The ATMega328P clock runs at 16MHz and AVR ADC needs between 50-200kHz
     * for accurate readings. 16MHz / 128 = 125kHz which is in that range.
     */
    ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);  // Set prescaler value to 128.
}

static uint16_t adc_read(const uint8_t channel) {
    // Select channel 0-7.
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

    // Start the conversion:
    ADCSRA |= _BV(ADSC);

    // Wait for the conversion to finish:
    while (ADCSRA & _BV(ADSC));

    return ADC;
}

static void timer1_init(void) {
    TCCR1A = 0;
    TCCR1B = _BV(WGM12)             // Clear-Timer-on-Compare (CTC) Mode.
        | _BV(CS12);                // Set prescaler to 256.

    /*
     * F_CPU (CPU frequency) is 16MHz.
     * Prescaler value is 256.
     * F_CPU / Prescaler = 16MHz / 256 = 62.5kHz.
     * Tick length = 1 / (F_CPU / Prescaler) = 1 / 62.5kHz = 16us.
     * Thus, the number of ticks to wait is equal to (milliseconds to wait) / 16us.
     * For a 500ms interval, ticks = 500ms / 16us = 31250 ticks (0-31249).
     */
    const uint16_t max_tick_value = 31249;
    OCR1A = max_tick_value;

    TIMSK1 |= _BV(OCIE1A);          // Enable Timer1 Compare Interrupt.
    sei();
}

static void gpio_init(void) {
    DDRB |= _BV(PIN_LCD_RS)
        | _BV(PIN_LCD_EN)
        | _BV(PIN_LCD_D4)
        | _BV(PIN_LCD_D5)
        | _BV(PIN_LCD_D6)
        | _BV(PIN_LCD_D7);

    DDRC &= ~_BV(PIN_TMP36_IN);
}

static volatile bool flag_adc_ready = false;
ISR(TIMER1_COMPA_vect) {
    flag_adc_ready = true;
}

extern bool tmp36_data_ready(void) {
    return flag_adc_ready;
}

extern uint16_t tmp36_read(void) {
    if (!flag_adc_ready)
        return -1;
    const uint8_t data = adc_read(PIN_TMP36_IN);
    flag_adc_ready = false;
    return data;
}

extern void app_main(void);

int main(void) {
    gpio_init();
    timer1_init();
    adc_init();
    display_init();
    app_main();
}