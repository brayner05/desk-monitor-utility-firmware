#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#define PIN_LCD_D7      PB5
#define PIN_LCD_D6      PB4
#define PIN_LCD_D5      PB3
#define PIN_LCD_D4      PB2
#define PIN_LCD_EN      PB1
#define PIN_LCD_RS      PB0
#define PIN_TMP32_IN    PC0

static int impl_put_char(char ch, FILE *stream __attribute__((unused)));
FILE display_stream_out = FDEV_SETUP_STREAM(impl_put_char, NULL, _FDEV_SETUP_WRITE);

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

static void display_clear(void) {
    display_execute(0x01);
}

static void display_goto(const uint8_t col, const uint8_t row) {
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
    if (ch == '\n') {
        display_goto(0, 1);
    }
    else if (ch == '\r') {
        display_execute(CMD_CURSOR_RETURN);
    }
    else {
        display_send_byte(ch, MODE_DATA);
    }
    return 0;
}

ISR(INT0) {

}

int main(void) {
    DDRB |= _BV(PIN_LCD_RS)
        | _BV(PIN_LCD_EN)
        | _BV(PIN_LCD_D4)
        | _BV(PIN_LCD_D5)
        | _BV(PIN_LCD_D6)
        | _BV(PIN_LCD_D7);

    DDRC &= ~_BV(PIN_TMP32_IN);;
    display_init();

    printf("Hello World!\rNONONO");

    while (1);
}