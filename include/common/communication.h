#include "hardware/uart.h"
#ifndef COMUNICATION_H
#define COMUNICATION_H


void read_from_uart(uart_inst_t* uart_id, uint8_t* buffer, int length) {
    int received = 0;
    while (received < length) {
        if (uart_is_readable(uart_id)) {
            buffer[received++] = uart_getc(uart_id);
        }
        sleep_ms(100); // Prevent busy waiting
        printf("Waiting for data... (%d/%d)\n", received, length);
    }
}

#endif // COMUNICATION_H
