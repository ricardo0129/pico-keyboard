const int KEY_EVENT_SIZE = 10; // 1 byte for is_pressed, 8 bytes for timestamp, 1 byte for keycode
struct KeyEvent {
    bool is_pressed;
    uint64_t timestamp;
    uint8_t keycode;
};

void seralize_key_event(const KeyEvent& event, uint8_t* buffer) {
    buffer[0] = event.is_pressed ? 1 : 0;
    for (int i = 0; i < 8; i++) {
        buffer[i + 1] = (event.timestamp >> (i * 8)) & 0xFF;
    }
    buffer[9] = event.keycode;
}

void deserialize_key_event(const uint8_t* buffer, KeyEvent& event) {
    event.is_pressed = buffer[0] == 1;
    event.timestamp = 0;
    for(int i = 0; i < 8; i++) {
        event.timestamp <<= 8;
        event.timestamp |= buffer[8 - i];
    }
    event.keycode = buffer[9];
}
