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
#include "common/keyboard.h"

#include <hardware/i2c.h>
#include <pico/i2c_slave.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common/circular_queue.h"

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


void process_key_press(bool is_pressed, uint64_t now, KeyState &key_state, uint8_t keycode) {
    if (is_pressed != key_state.is_pressed) {
        key_state.is_pressed = is_pressed;
        key_state.last_changed = now;
        if (is_pressed) {
            // Key pressed
            printf("Key %c pressed\n");
        } else {
            // Key released
            printf("Key %c released\n");
        }
    }
}

enum class State {
    RETURN_LENGTH,
    READ_REQUESTED_LENGTH,
    WRITE_REQUESTED_LENGTH
};

static struct {
    circular_queue<uint8_t> mem; // memory stack
    State current_state = State::RETURN_LENGTH;
    uint8_t write_remaining = 0; // number of bytes remaining to be written
} context;


// The child implements a 256 byte memory. To write a series of bytes, the parent first
// writes the memory address, followed by the data. The address is automatically incremented
// for each byte transferred, looping back to 0 upon reaching the end. Reading is done
// sequentially from the current memory address.

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



int main() {
    stdio_init_all();

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
        scan_keyboard(kb_left, process_key_press);
        sleep_ms(10);
    }
    return 0;
}
