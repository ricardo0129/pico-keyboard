#include "constants.h"
#include "pico/stdlib.h"
#include <stdint.h>

#ifndef CIRCULAR_QUEUE_H
#define CIRCULAR_QUEUE_H

template <typename T> struct circular_queue {
  T data[MAX_BUFFER_SIZE];
  volatile uint32_t tail, head;

  circular_queue();
  bool push(T value);
  T pop();
  const T &peek() const;
  uint32_t size();
};

// Template implementations added here because of linker errors

template <typename T> circular_queue<T>::circular_queue() : tail(0), head(0) {}

template <typename T> bool circular_queue<T>::push(T value) {
  uint32_t next_head = (head + 1) % MAX_BUFFER_SIZE;
  if (next_head == tail) {
    return false; // Queue is full
  }
  data[head] = value;
  head = next_head;
  return true; // Successfully pushed
}

template <typename T> T circular_queue<T>::pop() {
  if (tail == head) {
    hard_assert(false, "Queue is empty");
  }
  T value = data[tail];
  tail = (tail + 1) % MAX_BUFFER_SIZE;
  return value;
}

template <typename T> const T &circular_queue<T>::peek() const {
  if (tail == head) {
    hard_assert(false, "Queue is empty");
  }
  return data[tail];
}

template <typename T> uint32_t circular_queue<T>::size() {
  if (tail <= head) {
    return head - tail;
  }
  return MAX_BUFFER_SIZE - tail + head;
}

#endif // CIRCULAR_QUEUE_H
