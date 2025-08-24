#include <stdint.h>
#include "pico/stdlib.h"
#include "constants.h"

#ifndef CIRCULAR_QUEUE_H
#define CIRCULAR_QUEUE_H

template<typename T>
struct circular_queue {
    T data[MAX_BUFFER_SIZE];
    volatile uint32_t head, tail;

    circular_queue();
    bool push(T value);
    T pop();
    const T& peek() const;
    uint32_t size();

};


// Template implementations added here because of linker errors

template<typename T>
circular_queue<T>::circular_queue() : head(0), tail(0) {
}

template<typename T>
bool circular_queue<T>::push(T value) {
    uint32_t next_tail = (tail + 1) % MAX_BUFFER_SIZE;
    if(next_tail == head) {
        return false; // Queue is full
    }
    data[tail] = value;
    tail = next_tail;
    return true; // Successfully pushed
}

template<typename T>
T circular_queue<T>::pop() {
    if(head == tail) {
        hard_assert(false, "Queue is empty");
    }
    T value = data[head];
    head = (head + 1) % MAX_BUFFER_SIZE;
    return value;
}

template<typename T>
const T& circular_queue<T>::peek() const {
    if(head == tail) {
        hard_assert(false, "Queue is empty");
    }
    return data[head];
}

template<typename T>
uint32_t circular_queue<T>::size() {
    if(head <= tail) {
        return tail - head;
    }
    return MAX_BUFFER_SIZE - head + tail;
}


#endif // CIRCULAR_QUEUE_H
