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
#include "common/communication.h"

const int LED_PIN = 17;
#define UART_ID uart1
#define UART_TX_PIN 4
#define UART_RX_PIN 5

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
    bool debug_flag = false;
} context;

void process_key_press(bool is_pressed, uint64_t now, uint8_t keycode) {
    KeyEvent event = {
        .is_pressed = is_pressed,
        .timestamp = now,
        .keycode = keycode
    };
    context.mem.push(event);
    if(is_pressed && context.mem.size() > 0 && context.debug_flag) {
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        sleep_ms(100);
    }
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
                context.debug_flag = true;
                context.current_state = State::READ_REQUESTED_LENGTH;
            }
            else if(context.current_state == State::WRITE_REQUESTED_LENGTH) {
                if(context.ready_length == 0) {
                    if(context.write_remaining == 0) {
                        //We are done writing
                        context.current_state = State::RETURN_LENGTH;
                        break;
                    }
                    //We need to fill the buffer
                    serialize_key_event(context.mem.pop(), context.ready_buffer);
                    context.ready_length = KEY_EVENT_SIZE;
                    context.write_remaining--;
                }
                //We have data ready to send
                i2c_write_byte_raw(i2c, context.ready_buffer[KEY_EVENT_SIZE - context.ready_length]);
                context.ready_length--;
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

void initialize_uart() {
    // Initialize UART at 115200 baud
    uart_init(UART_ID, 115200);
    // Set GPIO functions for UART
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
}


int main() {
    stdio_init_all();
    const int LED_PIN = 17;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    printf("Starting UART test...\n");
    printf("Initializing UART...\n");
    initialize_uart();
    KeyEvent event = {
        .is_pressed = true,
        .timestamp = 123456789,
        .keycode = 'A'
    };
    uint8_t buf[KEY_EVENT_SIZE];
    serialize_key_event(event, buf);
    gpio_put(LED_PIN, 1);
    uart_puts(UART_ID, "Sending KeyEvent over UART...\n");
    uart_write_blocking(UART_ID, buf, KEY_EVENT_SIZE);
    printf("Original KeyEvent: is_pressed=%d, timestamp=%llu, keycode=%c\n", event.is_pressed, event.timestamp, event.keycode);
    for(int i = 0; i < KEY_EVENT_SIZE; i++) {
        printf("Byte %d: sent=0x%02X\n", i, buf[i]);
    }
    while(true) {
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        sleep_ms(100);
    }
    /*
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
    int row_to_pin[4] = {22, 26, 27, 20};
    int col_to_pin[5] = {9, 8, 7, 6, 5};
    KeyBoard kb_right(
        row_to_pin, 
        col_to_pin,
        right_layout_vec,
        4, // rows
        5  // cols
    );
    initalize_keyboard(kb_right);
    printf("Keyboard left initialized\n");


    while(true) {
        printf("Polling I2C child...\n");
        scan_keyboard(kb_right, process_key_press);
        sleep_ms(10);
    }
    */
    return 0;
}
