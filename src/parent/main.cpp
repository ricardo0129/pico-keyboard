#include <unordered_map>
#include <string.h>
#include <time.h>
#include "common/mylib.h"
#include "pico/stdlib.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bsp/board_api.h"
#include "tusb.h"
#include "common/usb_descriptors.h"
#include "common/constants.h"
#include "common/keyboard.h"
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common/key_event.h"
#include "common/circular_queue.h"
#include "common/keyboard_layout.h"
#include <map>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "common/communication.h"
#include "common/proto.h"



uint8_t const conv_table[128][2] =  { HID_ASCII_TO_KEYCODE };

static circular_queue<KeyEvent> key_event_queue;
const int LED_PIN = 17;

uint8_t buf[MAX_BUFFER_SIZE];

void process_key_press(bool is_pressed, uint64_t now, uint8_t keycode, std::map<uint8_t, KeyState>& keystate) {
    KeyState current = keystate[keycode];
    if(is_pressed != current.is_pressed) { //keystate has changed, add to processing queue
        KeyEvent event = {
            .is_pressed = is_pressed,
            .keycode = keycode
        };
        key_event_queue.push(event);
        keystate[keycode].is_pressed = is_pressed;
        keystate[keycode].last_changed = now;
    }
}

void run_parent() {
    uint8_t buf[KEY_EVENT_SIZE];
    bool valid = read_from_uart(UART_ID, buf, KEY_EVENT_SIZE);
    if(!valid) {
        //Timeout reading from UART
        return;
    }
    uint8_t requested_length = buf[0];
    if(requested_length > 0) {
        KeyEvent event;
        for(int i = 0; i < requested_length; i++) {
            valid = read_from_uart(UART_ID, buf, KEY_EVENT_SIZE);
            if(!valid) {
                //Timeout reading from UART
                return;
            }
            deserialize_key_event(buf, event);
            key_event_queue.push(event);
        }
    }

}


void read_events() {
}

void hid_task(void);
void init_tusb() {
    board_init();
    // init device stack on configured roothub port
    tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO
    };
    tusb_init(BOARD_TUD_RHPORT, &dev_init);

    if (board_init_after_tusb) {
        board_init_after_tusb();
    }
}

 
//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
}


static void send_hid_report(uint8_t report_id) {
    // skip if hid is not ready yet
    if (!tud_hid_ready()) 
        return;
    uint8_t keycode[MAX_KEY_COUNT] = { 0 };
    uint8_t key_count = 0;
    uint8_t modifier  = 0;
    int queue_size = key_event_queue.size();

    for(int i = 0; i < queue_size && key_count < MAX_KEY_COUNT; i++) {
        KeyEvent event = key_event_queue.pop();
        uint8_t current_modifier = 0;
        if(conv_table[event.keycode][0]) {
            current_modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        }
        if(i == 0) {
            modifier = current_modifier;
        }
        else if(current_modifier != modifier) {
            //Modifier has changed, stop processing events
            break;
        }
        if(event.is_pressed) {
            //Key is pressed
            keycode[key_count++] = conv_table[event.keycode][1];
        }
    }
    //some events must have been processed
    if(queue_size) {
        if(key_count) {
            tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, keycode);
        }
        else {
            tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
        }
    }
    /*
    if(state != key_states[i][j].is_pressed && now - key_states[i][j].last_changed > DEBOUNCE_US) {
        key_states[i][j].is_pressed = state;
        key_states[i][j].last_changed = now;
        uint8_t chr = left_layout[i][j];
        if( conv_table[chr][0] ) modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        if(state && key_count < MAX_KEY_COUNT) {
            keycode[key_count++] = conv_table[chr][1];
            changed = true;
        }
    }
    else if(state && now - key_states[i][j].last_changed > KEY_HOLD_US && key_count < MAX_KEY_COUNT) {
        uint8_t chr = left_layout[i][j];
        if( conv_table[chr][0] ) modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        keycode[key_count++] = conv_table[chr][1];
        changed = true;
    }
    */
}


// Every 10ms, we will sent 1 report for each HID profile (keyboard, mouse etc ..)
// tud_hid_report_complete_cb() is used to send the next report after previous one is complete
void hid_task(void) {
    // Poll every 10ms
    const uint32_t interval_ms = 10;
    static uint32_t start_ms = 0;
    static uint32_t touch_ms = 0;
    static bool touch_state = false;
    if(board_millis() - start_ms < interval_ms) 
        return; // not enough time
    start_ms += interval_ms;

    //uint32_t const btn = board_button_read();

    // Remote wakeup
    if(tud_suspended()) {
        // Wake up host if we are in suspend mode
        // and REMOTE_WAKEUP feature is enabled by host
        tud_remote_wakeup();
    }
    else {
        // Send the 1st of report chain, the rest will be sent by tud_hid_report_complete_cb()
        send_hid_report(REPORT_ID_KEYBOARD);
    }
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len) {
    (void) instance;
    (void) len;
    uint8_t next_report_id = report[0] + 1u;

    /*
    if (next_report_id < REPORT_ID_COUNT) {
        send_hid_report(next_report_id);
    }
    */
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    // TODO not Implemented
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    (void) instance;

    /*
    if(report_type == HID_REPORT_TYPE_OUTPUT) {
    // Set keyboard LED e.g Capslock, Numlock etc...
        if (report_id == REPORT_ID_KEYBOARD) {
        // bufsize should be (at least) 1
            if( bufsize < 1 ) 
                return;
            uint8_t const kbd_leds = buffer[0];
            if(kbd_leds & KEYBOARD_LED_CAPSLOCK) {
                // Capslock On: disable blink, turn led on
                blink_interval_ms = 0;
                board_led_write(true);
            }
            else {
                // Caplocks Off: back to normal blink
                board_led_write(false);
                blink_interval_ms = BLINK_MOUNTED;
            }
        }
    }
    */
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
    initialize_uart();

    /*
    frame_t tx, rx;
    uint8_t seq = 0;
    uint8_t buff[256];
    
    KeyEvent event = {
        .is_pressed = true,
        .keycode = 'A'
    };

    //take an event and send it
    serialize_key_event(event, buff);
    proto_encode(&tx, buff, TYPE_DATA, KEY_EVENT_SIZE, seq++);
    for(int i = 0; i < 10; i++) {
        send_frame(&tx);
    }

    KeyEvent received_event;
    int j = 0;
    while(true) {
        if(recv_frame(&rx)) {
            deserialize_key_event(rx.data, received_event); 
            //printf("Got frame: seq=%d, len=%d", rx.seq, rx.len);
            printf("%d: Event deserialized: is_pressed: %d keycode: %c\n", j, received_event.is_pressed, received_event.keycode);
            j += 1;
        }
    }
    */
    int row_to_pin[4] = {27, 28, 26, 22};
    int col_to_pin[5] = {21, 4, 5, 6, 7};
    KeyBoard kb_left(
        row_to_pin, 
        col_to_pin,
        left_layout_vec,
        4, // rows
        5  // cols
    );
    printf("Keyboard left initialized\n");

    initalize_keyboard(kb_left);


    init_tusb();

    while(1) {
        tud_task(); // tinyusb device task
        scan_keyboard(kb_left, process_key_press);
        run_parent();
        hid_task();
        sleep_ms(10); // sleep for 10 ms
    }
    return 0;
}
