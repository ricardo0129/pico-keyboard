#include <unordered_map>
#include <string.h>
#include <time.h>
#include "pico_net/mylib.h"

#include "pico/stdlib.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board_api.h"
#include "tusb.h"

#include "usb_descriptors.h"

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

#ifdef i2c_default
// For this example, we run both the parent and child from the same board.
// You'll need to wire pin GP4 to GP6 (SDA), and pin GP5 to GP7 (SCL).
static const uint I2C_CHILD_SDA_PIN = PICO_DEFAULT_I2C_SDA_PIN; // 4
static const uint I2C_CHILD_SCL_PIN = PICO_DEFAULT_I2C_SCL_PIN; // 5
static const uint I2C_PARENT_SDA_PIN = 6;
static const uint I2C_PARENT_SCL_PIN = 7;

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

static void run_parent() {
    gpio_init(I2C_PARENT_SDA_PIN);
    gpio_set_function(I2C_PARENT_SDA_PIN, GPIO_FUNC_I2C);
    // pull-ups are already active on child side, this is just a fail-safe in case the wiring is faulty
    gpio_pull_up(I2C_PARENT_SDA_PIN);

    gpio_init(I2C_PARENT_SCL_PIN);
    gpio_set_function(I2C_PARENT_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_PARENT_SCL_PIN);

    i2c_init(i2c1, I2C_BAUDRATE);

    uint8_t buf[MAX_BUFFER_SIZE];
    int i = 0;
    while(true) {
        for(int j = 0; j < i%3; j++) {
            context.mem.push(j);
        }
        printf("%d items in memory\n", context.mem.size());
        printf("current state: %d\n", context.current_state);
        printf("Requesting length from child...\n");
        int count = i2c_read_blocking(i2c1, I2C_CHILD_ADDRESS, buf, 1, true);
        if (count < 0) {
            puts("Couldn't read from child, please check your wiring!");
            return;
        }
        hard_assert(count == 1, "Expected to read 1 byte");
        uint8_t requested_length = buf[0];
        hard_assert(requested_length <= MAX_BUFFER_SIZE, "Requested length exceeds buffer size");
        printf("current state: %d\n", context.current_state);
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
        sleep_ms(2000);
        i++;
    }
}
#endif





const int LED_PIN = 17;

const int COLS = 5;
const int ROWS = 4;

const int ROW_TO_PIN[4] = {27, 28, 26, 22};
const int COL_TO_PIN[5] = {21, 4, 5, 6, 7};

struct KeyState {
    bool is_pressed;
    uint64_t last_changed;
};

KeyState key_states[ROWS][COLS] = {0};

char left_layout[3][5] = {
    {'T', 'R', 'E', 'W', 'Q'},
    {'G', 'F', 'D', 'S', 'A'},
    {'B', 'V', 'C', 'X', 'Z'},
};

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

uint8_t const conv_table[128][2] =  { HID_ASCII_TO_KEYCODE };
 


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

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+
static uint32_t cc = 0;
const int MAX_KEY_COUNT = 6;
const uint64_t DEBOUNCE_US = 5000;
const uint64_t KEY_HOLD_US = 300000; // 300ms

static void send_hid_report(uint8_t report_id) {
    // skip if hid is not ready yet
    if (!tud_hid_ready()) 
        return;
    static bool changed = false;
    uint8_t keycode[MAX_KEY_COUNT] = { 0 };
    uint8_t key_count = 0;
    uint8_t modifier  = 0;

    for(int i = 0; i < ROWS; i++) {
        int row_pin = ROW_TO_PIN[i];
        gpio_put(row_pin, 1);
        sleep_ms(1);
        for(int j = 0; j < COLS; j++) {
            int col_pin = COL_TO_PIN[j];
            int state = gpio_get(col_pin);
            uint64_t now = time_us_64();
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
        }
        gpio_put(row_pin, 0);
    }
    if(changed) {
        if(key_count) {
            tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, keycode);
        }
        else {
            tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
        }
    }

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

//map<int,int> rows_in_col[COLS];
void init_keyboard_pins() {

    /*
    while(true) {
        if(value == 1) {
        }
        sleep_ms(1);
    }
    */
}

void scan_matrix() {
    init_keyboard_pins();
}


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
    gpio_put(LED_PIN, 1);

    setup_child();
    run_parent();
#endif
    while(true) {
        sleep_ms(1000);
    }
    return 0;

    for(int i = 0; i < ROWS; i++) {
        for(int j = 0; j < COLS; j++) {
            key_states[i][j].is_pressed = false;
            key_states[i][j].last_changed = time_us_64();
        }
    }

    // Turn on LED to indicate we are running

    for(int i = 0; i < COLS; i++) {
        int pin = COL_TO_PIN[i];
        gpio_init(pin);
        //gpio_set_dir(pin, GPIO_OUT);
        gpio_set_dir(pin, GPIO_IN);
    }
    for(int i = 0; i < ROWS; i++) {
        int pin = ROW_TO_PIN[i];
        gpio_init(pin);
        //gpio_set_dir(pin, GPIO_IN);
        gpio_set_dir(pin, GPIO_OUT);
    }


    init_tusb();


    while(1) {
        tud_task(); // tinyusb device task
        hid_task();
        sleep_ms(10); // sleep for 1 second
    }

    const std::unordered_map<std::string, greeter::LanguageCode> languages{
        {"en", greeter::LanguageCode::EN},
        {"de", greeter::LanguageCode::DE},
        {"es", greeter::LanguageCode::ES},
        {"fr", greeter::LanguageCode::FR},
    };
    std::string language = "en";

    auto langIt = languages.find(language);

    greeter::Greeter greeter("RICKY");
    printf("{}\n", greeter.greet(langIt->second));
    return 0;
}
