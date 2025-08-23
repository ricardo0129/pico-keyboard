#include "common/keyboard.h"
#include <stdio.h>

KeyBoard::KeyBoard(int* _row_to_pin, int* _col_to_pin,std::vector<std::vector<char>> row_layout,int _rows, int _cols) {
    row_to_pin = _row_to_pin;
    col_to_pin = _col_to_pin;
    row_layout = row_layout;
    rows = _rows;
    cols = _cols;
    uint64_t now = time_us_64();
    bool is_pressed = false;
    key_states.assign(rows, std::vector<KeyState>(cols, {.is_pressed = is_pressed, .last_changed = now}));
}

int KeyBoard::cols_at_row(int row) {
    if(row < 0 || row >= rows) {
        return -1; // Invalid row
    }
    return row_layout[row].size();
}

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

void scan_keyboard(KeyBoard& kb, void (*func)(bool, uint64_t, KeyState&, uint8_t)) {
    for(int i = 0; i < kb.rows; i++) {
        gpio_put(kb.row_to_pin[i], 1); // Set the row pin high
        sleep_ms(1); // Allow time for the signal to stabilize
        for(int j = 0; j < kb.cols_at_row(i); j++) {
            int col_pin = kb.col_to_pin[j];
            uint64_t now = time_us_64();
            bool is_pressed = !gpio_get(col_pin); // Active low
            uint8_t keycode = kb.row_layout[i][j];
            func(is_pressed, now, kb.key_states[i][j], keycode); // Call the function with the key state
            /*
            if(is_pressed != kb.key_states[i][j].is_pressed) {
                kb.key_states[i][j].is_pressed = is_pressed;
                kb.key_states[i][j].last_changed = now;
                txq.push(kb.row_layout[i][j]); // Push the key to the queue
                printf("Key %c at (%d, %d) %s\n", kb.row_layout[i][j], i, j, is_pressed ? "pressed" : "released");
            }
            */
        }
        gpio_put(kb.row_to_pin[i], 0); // Set the row pin low
    }
}

