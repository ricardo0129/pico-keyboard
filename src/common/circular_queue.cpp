#include "common/circular_queue.h"
#include "pico/stdlib.h"

circular_queue::circular_queue() : head(0), tail(0) {
}

bool circular_queue::push(uint8_t value) {
    if((head + 1) % MAX_BUFFER_SIZE == tail) {
        return false; // Queue is full
    }
    data[tail] = value;
    tail = (tail + 1) % MAX_BUFFER_SIZE;
    return true; // Successfully pushed
}

uint8_t circular_queue::pop() {
    if(head == tail) {
        hard_assert(false, "Queue is empty");
    }
    uint8_t value = data[head];
    head = (head + 1) % MAX_BUFFER_SIZE;
    return value;
}

uint32_t circular_queue::size() {
    if(head <= tail) {
        return tail - head;
    }
    return MAX_BUFFER_SIZE - head + tail;
}

