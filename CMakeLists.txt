cmake_minimum_required(VERSION 3.20)
project(Project C)

# Tell CMake that this project is not for the host system:
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)

# MCU and clock
set(MCU atmega328p)
set(F_CPU 16000000UL)

# AVR compiler
set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_CXX_COMPILER avr-g++)
set(CMAKE_ASM_COMPILER avr-gcc)

# Compiler flags
set(CMAKE_C_FLAGS "-mmcu=${MCU} -DF_CPU=${F_CPU} -Os")
set(CMAKE_EXE_LINKER_FLAGS "-mmcu=${MCU}")

add_executable(${PROJECT_NAME}.elf src/main.c)

add_custom_command(
        TARGET ${PROJECT_NAME}.elf
        POST_BUILD
        COMMAND avr-objcopy -O ihex -R .eeprom
        ${PROJECT_NAME}.elf
        ${PROJECT_NAME}.hex
)
