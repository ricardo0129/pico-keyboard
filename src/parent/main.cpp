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

uint8_t buf[MAX_BUFFER_SIZE];

void process_key_press(bool is_pressed, uint64_t now, KeyState &key_state, uint8_t keycode) {
    if (is_pressed != key_state.is_pressed) {
        key_state.is_pressed = is_pressed;
        key_state.last_changed = now;
        if (is_pressed) {
            // Key pressed
        } else {
            // Key released
        }
    }
}

void initalize_parent() {
    gpio_init(I2C_PARENT_SDA_PIN);
    gpio_set_function(I2C_PARENT_SDA_PIN, GPIO_FUNC_I2C);
    // pull-ups are already active on child side, this is just a fail-safe in case the wiring is faulty
    gpio_pull_up(I2C_PARENT_SDA_PIN);

    gpio_init(I2C_PARENT_SCL_PIN);
    gpio_set_function(I2C_PARENT_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_PARENT_SCL_PIN);

    i2c_init(i2c1, I2C_BAUDRATE);
}

static void run_parent() {
    int count = i2c_read_blocking(i2c1, I2C_CHILD_ADDRESS, buf, 1, true);
    if (count < 0) {
        puts("Couldn't read from child, please check your wiring!");
        return;
    }
    hard_assert(count == 1, "Expected to read 1 byte");
    uint8_t requested_length = buf[0];
    hard_assert(requested_length <= MAX_BUFFER_SIZE, "Requested length exceeds buffer size");
    printf("Requesting %d bytes from child...\n", requested_length);
    if(requested_length > 0) {
        i2c_write_blocking(i2c1, I2C_CHILD_ADDRESS, buf, 1, true);
        printf("Reading %d bytes from child...\n", buf[0]);
        //read all the requested bytes
        count = i2c_read_blocking(i2c1, I2C_CHILD_ADDRESS, buf, requested_length, false);
        hard_assert(count == requested_length, "Expected to read %d bytes, got %d", requested_length, count);
    }
    else {
        printf("Requested length is zero, nothing to write\n");
        i2c_write_blocking(i2c1, I2C_CHILD_ADDRESS, buf, 1, false);
        printf("Done writing\n");
    }
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

struct KeyEvent {
    bool is_pressed;
    uint64_t timestamp;
    uint8_t keycode;
};

int events_in_queue = 0;
KeyEvent key_events[256];


static void send_hid_report(uint8_t report_id) {
    // skip if hid is not ready yet
    if (!tud_hid_ready()) 
        return;
    static bool changed = false;
    uint8_t keycode[MAX_KEY_COUNT] = { 0 };
    uint8_t key_count = 0;
    uint8_t modifier  = 0;
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
    if(changed) {
        if(key_count) {
            tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, keycode);
        }
        else {
            tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
        }
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

int main() {
    stdio_init_all();

#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
#warning i2c / child_mem_i2c example requires a board with I2C pins
    puts("Default I2C pins were not defined");
    return 0;
#else
    run_parent();
#endif

    init_tusb();

    while(1) {
        tud_task(); // tinyusb device task
        hid_task();
        sleep_ms(10); // sleep for 10 ms
    }
    return 0;
}
