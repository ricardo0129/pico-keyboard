#include "hardware/uart.h"
#ifndef COMUNICATION_H
#define COMUNICATION_H


bool read_from_uart(uart_inst_t* uart_id, uint8_t* buffer, int length) {
    uint64_t start_time = time_us_64();
    int received = 0;
    while (received < length) {
        // Wait until data is available
        while (uart_is_readable(uart_id)) {
            buffer[received++] = uart_getc(uart_id);
        }
        // Read one byte
        //tight_loop_contents(); // Low-power wait
        if(time_us_64() - start_time > 100000) {
            // Timeout
            return false;
        }
    }
    return true;
}

#endif // COMUNICATION_H
