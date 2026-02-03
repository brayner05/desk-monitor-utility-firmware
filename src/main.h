#ifndef MAIN_H
#define MAIN_H
#include <stdbool.h>

#define GLYPH_TEMPERATURE   1
#define GLYPH_WIFI          2
#define GLYPH_CLOCK         3
#define GLYPH_BRIGHTNESS    4

extern void display_clear(void);
extern void display_goto(const uint8_t col, const uint8_t row);
extern bool tmp36_data_ready(void);
extern uint16_t tmp36_read(void);
extern bool photoresistor_data_ready(void);
extern uint16_t photoresistor_read(void);

#endif // MAIN_H