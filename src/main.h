#ifndef AVR_CMAKE_MAIN_H
#define AVR_CMAKE_MAIN_H
#include <stdbool.h>

extern void display_clear(void);
extern void display_goto(const uint8_t col, const uint8_t row);
extern bool tmp36_data_ready(void);
extern uint16_t tmp36_read(void);

#endif //AVR_CMAKE_MAIN_H