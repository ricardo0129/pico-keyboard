const int KEY_EVENT_SIZE = 2; // 1 byte for is_pressed, 1 byte for keycode
struct KeyEvent {
    bool is_pressed;
    uint8_t keycode;
};

void serialize_key_event(const KeyEvent& event, uint8_t* buffer) {
    buffer[0] = event.is_pressed ? 1 : 0;
    buffer[1] = event.keycode;
}

void deserialize_key_event(const uint8_t* buffer, KeyEvent& event) {
    event.is_pressed = buffer[0] == 1;
    event.keycode = buffer[1];
}
