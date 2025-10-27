#include "common/mylib.h"
#include "pico/stdlib.h"
#include <stdlib.h>
#include <stdio.h>
#include "bsp/board.h"
#include "bsp/board_api.h"
#include "tusb.h"
#include "common/usb_descriptors.h"
#include "common/keyboard.h"
#include <pico/stdlib.h>
#include <stdio.h>
#include "common/circular_queue.h"
#include "common/keyboard_layout.h"
#include "common/key_event.h"
#include "common/communication.h"

const int LED_PIN = 17;
void blink() {
    gpio_put(LED_PIN, 1);
    sleep_ms(100);
    gpio_put(LED_PIN, 0);
    sleep_ms(100);
}
uint8_t seq = 0;


static struct {
    circular_queue<KeyEvent> mem; // memory stack
} context;

void process_key_press(bool is_pressed, uint64_t now, uint8_t keycode, std::map<uint8_t, KeyState>& keystate) {
    KeyState current = keystate[keycode];
    if(is_pressed != current.is_pressed) { //keystate has changed, report to left side
        KeyEvent event = {
            .is_pressed = is_pressed,
            .keycode = keycode
        };
        context.mem.push(event);
        keystate[keycode].is_pressed = is_pressed;
        keystate[keycode].last_changed = now;
    }
}

void write_events() {
    uint queue_size = context.mem.size();
    uint8_t buf[KEY_EVENT_SIZE];
    frame_t tx;
    for(int i = 0; i < queue_size; i++) {
        KeyEvent e = context.mem.pop();
        serialize_key_event(e, buf);
        proto_encode(&tx, buf, TYPE_DATA, KEY_EVENT_SIZE, seq++);
        send_frame(&tx);
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
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
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
        write_events();
        sleep_ms(10); //TODO Change after debugging
    }
    return 0;
}
