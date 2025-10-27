#include <map>
#include <pico/stdlib.h>
#include <stdint.h>
#include <vector>

#ifndef KEYBOARD_H
#define KEYBOARD_H

struct KeyState {
  bool is_pressed;
  uint64_t last_changed;
};

struct KeyBoard {
  int *row_to_pin;
  int *col_to_pin;
  int rows, cols;
  std::vector<std::vector<char>> row_layout;
  std::map<uint8_t, KeyState> *keystate;

  KeyBoard(int *_row_to_pin, int *_col_to_pin,
           std::vector<std::vector<char>> row_layout, int _rows, int _cols);

  int cols_at_row(int row);
};

void initalize_keyboard(KeyBoard &kb);

void scan_keyboard(KeyBoard &kb, void (*func)(bool, uint64_t, uint8_t,
                                              std::map<uint8_t, KeyState> &));

#endif // KEYBOARD_H
