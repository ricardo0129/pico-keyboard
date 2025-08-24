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
#include "common/keyboard_layout.h"
#include "common/key_event.h"

enum class State {
    RETURN_LENGTH,
    READ_REQUESTED_LENGTH,
    WRITE_REQUESTED_LENGTH
};

static struct {
    circular_queue<KeyEvent> mem; // memory stack
    State current_state = State::RETURN_LENGTH;
    uint8_t write_remaining = 0; // number of bytes remaining to be written
    uint8_t ready_buffer[MAX_BUFFER_SIZE];
    uint8_t ready_length = 0;
} context;

void process_key_press(bool is_pressed, uint64_t now, uint8_t keycode) {
    KeyEvent event = {
        .is_pressed = is_pressed,
        .timestamp = now,
        .keycode = keycode
    };
    context.mem.push(event);
}


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
                    //we got the requested number of objects to write
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
                if(context.ready_length == 0) {
                    //We need to fill the buffer
                    serialize_key_event(context.mem.pop(), context.ready_buffer);
                    context.ready_length = KEY_EVENT_SIZE;
                    context.write_remaining--;
                }
                if(context.write_remaining < 0) {
                    context.current_state = State::RETURN_LENGTH;
                } else {
                    //We have data ready to send
                    i2c_write_byte_raw(i2c, context.ready_buffer[KEY_EVENT_SIZE - context.ready_length]);
                    context.ready_length--;
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
    int row_to_pin[4] = {22, 26, 27, 20};
    int col_to_pin[5] = {9, 8, 7, 6, 5};
    KeyBoard kb_right(
        row_to_pin, 
        col_to_pin,
        right_layout_vec,
        4, // rows
        5  // cols
    );
    printf("Keyboard left initialized\n");


    while(true) {
        printf("Polling I2C child...\n");
        scan_keyboard(kb_right, process_key_press);
        sleep_ms(10);
    }
    return 0;
}
