#include <stdint.h>

#ifndef CONSTANTS_H
#define CONSTANTS_H

static const uint32_t I2C_CHILD_ADDRESS = 0x17;
static const uint32_t I2C_BAUDRATE = 100000; // 100 kHz
const int32_t MAX_QUEUE_SIZE = 256;
const int32_t MAX_BUFFER_SIZE = 32;

static const uint32_t I2C_CHILD_SDA_PIN = 2;
static const uint32_t I2C_CHILD_SCL_PIN = 3;

static const uint32_t I2C_PARENT_SDA_PIN = 2;
static const uint32_t I2C_PARENT_SCL_PIN = 3;
//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+
const int MAX_KEY_COUNT = 6;
const uint64_t DEBOUNCE_US = 5000;
const uint64_t KEY_HOLD_US = 300000; // 300ms
#endif                               // CONSTANTS_H

/* Pinout
 * Bat  RGB
 * GND  DATA
 * RST  GND
 * VCC  GND
 * LR   SDA
 * R1   SCL
 * R0   C1
 * R2   C2
 * R3   C3
 * E2A  C4
 * E2B  E1B
 * C0   E1A
 */
