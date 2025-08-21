#include <unordered_map>
#include <string.h>
#include <time.h>
#include "common/mylib.h"

#include "pico/stdlib.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "bsp/board.h"
#include "bsp/board_api.h"
#include "tusb.h"

#include "common/usb_descriptors.h"

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


/*
 * Copyright (c) 2021 Valentin Milea <valentin.milea@gmail.com>
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <hardware/i2c.h>
#include <pico/i2c_slave.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>

static const uint I2C_CHILD_ADDRESS = 0x17;
static const uint I2C_BAUDRATE = 100000; // 100 kHz

// For this example, we run both the parent and child from the same board.
// You'll need to wire pin GP4 to GP6 (SDA), and pin GP5 to GP7 (SCL).
static const uint I2C_CHILD_SDA_PIN = PICO_DEFAULT_I2C_SDA_PIN; // 4
static const uint I2C_CHILD_SCL_PIN = PICO_DEFAULT_I2C_SCL_PIN; // 5

enum class State {
    RETURN_LENGTH,
    READ_REQUESTED_LENGTH,
    WRITE_REQUESTED_LENGTH
};
const int MAX_QUEUE_SIZE = 256;
const int MAX_BUFFER_SIZE = 32;
struct stack {
    uint8_t data[256];
    uint8_t head;
    stack() : head(0) {
    }
    int push(uint8_t value) {
        if(head >= MAX_QUEUE_SIZE) {
            return -1; // stack is full
        }
        return data[head++] = value;
    }
    int pop() {
        if(head <= 0) {
            hard_assert(false, "Queue is empty");
        }
        return data[head--];
    }
    int size() {
        return head;
    }
};

// The child implements a 256 byte memory. To write a series of bytes, the parent first
// writes the memory address, followed by the data. The address is automatically incremented
// for each byte transferred, looping back to 0 upon reaching the end. Reading is done
// sequentially from the current memory address.
static struct {
    stack mem; // memory stack
    State current_state = State::RETURN_LENGTH;
    uint8_t write_remaining = 0; // number of bytes remaining to be written
} context;

// Our handler is called from the I2C ISR, so it must complete quickly. Blocking calls /
// printing to stdio may interfere with interrupt handling.
static void i2c_child_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {
    switch (event) {
        case I2C_SLAVE_RECEIVE: // parent has written some data
            //Parent has made a request for data
            if(context.current_state == State::READ_REQUESTED_LENGTH) {
                context.write_remaining = i2c_read_byte_raw(i2c);
                if(context.write_remaining == 0) {
                    //We can skip the next state
                    context.current_state = State::RETURN_LENGTH;
                } 
                else {
                    context.current_state = State::WRITE_REQUESTED_LENGTH;
                }
            }
            else {
                panic("Unexpected state while receiving data");
            }
            break;
        case I2C_SLAVE_REQUEST: // parent is requesting data
            if(context.current_state == State::RETURN_LENGTH) {
                // Master is requestion the size of the memory
                i2c_write_byte_raw(i2c, context.mem.size());
                context.current_state = State::READ_REQUESTED_LENGTH;
            }
            else if(context.current_state == State::WRITE_REQUESTED_LENGTH) {
                i2c_write_byte_raw(i2c, context.mem.pop());
                context.write_remaining--;
                if(context.write_remaining == 0) {
                    context.current_state = State::RETURN_LENGTH;
                }
            }
            else {
                // This should never happen
                panic("Unexpected state in while requesting data");
            }
            break;
        case I2C_SLAVE_FINISH: // parent has signalled Stop / Restart
            //Clean up the state
            break;
        default:
            break;
    }
}

static void setup_child() {
    gpio_init(I2C_CHILD_SDA_PIN);
    gpio_set_function(I2C_CHILD_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_CHILD_SDA_PIN);

    gpio_init(I2C_CHILD_SCL_PIN);
    gpio_set_function(I2C_CHILD_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_CHILD_SCL_PIN);

    i2c_init(i2c0, I2C_BAUDRATE);
    // configure I2C0 for child mode
    i2c_slave_init(i2c0, I2C_CHILD_ADDRESS, &i2c_child_handler);
}



const int LED_PIN = 17;


struct KeyState {
    bool is_pressed;
    uint64_t last_changed;
};

struct KeyBoard {
    int* row_to_pin;
    int* col_to_pin;
    int rows, cols;
    std::vector<std::vector<char>> row_layout;
    std::vector<std::vector<KeyState>> key_states;

    KeyBoard(int* _row_to_pin, int* _col_to_pin,std::vector<std::vector<char>> row_layout,int _rows, int _cols) {
        row_to_pin = _row_to_pin;
        col_to_pin = _col_to_pin;
        row_layout = row_layout;
        rows = _rows;
        cols = _cols;
        uint64_t now = time_us_64();
        bool is_pressed = false;
        key_states.assign(rows, std::vector<KeyState>(cols, {.is_pressed = is_pressed, .last_changed = now}));
    }

    int cols_at_row(int row) {
        if(row < 0 || row >= rows) {
            return -1; // Invalid row
        }
        return row_layout[row].size();
    }
};

void initalize_keyboard(KeyBoard& kb) {
    for(int i = 0; i < kb.cols; i++) {
        int pin = kb.col_to_pin[i];
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }
    for(int i = 0; i < kb.rows; i++) {
        int pin = kb.row_to_pin[i];
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
    }
}

void scan_keyboard(KeyBoard& kb) {
    for(int i = 0; i < kb.rows; i++) {
        gpio_put(kb.row_to_pin[i], 1); // Set the row pin high
        sleep_ms(1); // Allow time for the signal to stabilize
        for(int j = 0; j < kb.cols_at_row(i); j++) {
            int col_pin = kb.col_to_pin[j];
            uint64_t now = time_us_64();
            bool is_pressed = !gpio_get(col_pin); // Active low
            if(is_pressed != kb.key_states[i][j].is_pressed) {
                kb.key_states[i][j].is_pressed = is_pressed;
                kb.key_states[i][j].last_changed = now;
                //printf("Key %c at (%d, %d) %s\n", kb.row_layout[i][j], i, j, is_pressed ? "pressed" : "released");
            }
        }
        gpio_put(kb.row_to_pin[i], 0); // Set the row pin low
    }
}

char left_layout[3][5] = {
    {'T', 'R', 'E', 'W', 'Q'},
    {'G', 'F', 'D', 'S', 'A'},
    {'B', 'V', 'C', 'X', 'Z'},
};
char right_layout[3][5] = {
    {'Y', 'U', 'I', 'O', 'P'},
    {'H', 'J', 'K', 'L', ';'},
    {'N', 'M', ',', '.', '/'},
};

uint8_t const conv_table[128][2] =  { HID_ASCII_TO_KEYCODE };
 

int main() {
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
#warning i2c / child_mem_i2c example requires a board with I2C pins
    puts("Default I2C pins were not defined");
    return 0;
#else
    puts("\nI2C child example");
    setup_child();
#endif
    int row_to_pin[4] = {27, 28, 26, 22};
    int col_to_pin[5] = {21, 4, 5, 6, 7};
    std::vector<std::vector<char>> left_layout_vec = {
        {'T', 'R', 'E', 'W', 'Q'},
        {'G', 'F', 'D', 'S', 'A'},
        {'B', 'V', 'C', 'X', 'Z'},
        {' ', 'X'}
    };
    KeyBoard kb_left(
        row_to_pin, 
        col_to_pin,
        left_layout_vec,
        4, // rows
        5  // cols
    );
    printf("Keyboard left initialized\n");


    while(true) {
        printf("Polling I2C child...\n");
        scan_keyboard(kb_left);
        sleep_ms(10);
    }
    return 0;
}
