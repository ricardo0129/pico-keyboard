#include <vector>

uint8_t const conv_table[128][2] =  { HID_ASCII_TO_KEYCODE };

std::vector<std::vector<char>> left_layout_vec = {
    {'T', 'R', 'E', 'W', 'Q'},
    {'G', 'F', 'D', 'S', 'A'},
    {'B', 'V', 'C', 'X', 'Z'},
    {' ', '\n'}
};
std::vector<std::vector<char>> right_layout_vec = {
    {'Y', 'U', 'I', 'O', 'P'},
    {'H', 'J', 'K', 'L', ';'},
    {'N', 'M', ',', '.', '/'},
};
