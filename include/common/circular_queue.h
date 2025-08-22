#include <stdint.h>
#include "constants.h"

struct circular_queue {
    uint8_t data[MAX_BUFFER_SIZE];
    volatile uint32_t head, tail;

    circular_queue();
    bool push(uint8_t value);
    uint8_t pop();
    uint32_t size();

};
