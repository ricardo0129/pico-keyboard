#include <unordered_map>
#include <string.h>
#include <time.h>
#include "common/mylib.h"

#include "pico/stdlib.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "bsp/board_api.h"
#include "tusb.h"

#include "common/usb_descriptors.h"
#include "common/keyboard.h"

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


static struct {
    circular_queue<KeyEvent> mem; // memory stack
} context;

void process_key_press(bool is_pressed, uint64_t now, uint8_t keycode, std::map<uint8_t, KeyState>* keystate) {
    KeyEvent event = {
        .is_pressed = is_pressed,
        .timestamp = now,
        .keycode = keycode
    };
    KeyState current = (*keystate)[keycode];
    if(is_pressed != current.is_pressed) { //keystate has changed, report to left side
        context.mem.push(event);
    }
}


/*
enum class State {
    RETURN_LENGTH,
    READ_REQUESTED_LENGTH,
    WRITE_REQUESTED_LENGTH
};
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
*/

void initialize_uart() {
    // Initialize UART at 115200 baud
    uart_init(UART_ID, 115200);
    // Set GPIO functions for UART
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
}


int main() {
    stdio_init_all();
    /*
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    printf("Starting UART test...\n");
    printf("Initializing UART...\n");
    KeyEvent event = {
        .is_pressed = true,
        .timestamp = 123456789,
        .keycode = 'A'
    };
    uint8_t buf[KEY_EVENT_SIZE];
    while(1) {
        serialize_key_event(event, buf);
        uart_write_blocking(UART_ID, buf, KEY_EVENT_SIZE);
        uart_tx_wait_blocking(UART_ID);
        printf("Original KeyEvent: is_pressed=%d, timestamp=%llu, keycode=%c\n", event.is_pressed, event.timestamp, event.keycode);
        for(int i = 0; i < KEY_EVENT_SIZE; i++) {
            printf("Byte %d: sent=0x%02X\n", i, buf[i]);
        }
        sleep_ms(1000);
    }
    */
    //Initialize UART (communication)
    initialize_uart();
    printf("Initializing right side\n");
    //row/col -> gpio mappings
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
    printf("Keyboard right initialized\n");


    while(true) {
        scan_keyboard(kb_right, process_key_press);
        sleep_ms(1000); //TODO Change after debugging
    }
    return 0;
}
