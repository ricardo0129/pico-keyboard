#ifndef COMUNICATION_H
#define COMUNICATION_H

#include "hardware/uart.h"
#include "common/proto.h"

#define UART_ID uart1
#define BAUD 115200
#define UART_TX_PIN 4
#define UART_RX_PIN 5

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

void send_frame(frame_t* frame) {
    uart_putc_raw(UART_ID, frame->type);
    uart_putc_raw(UART_ID, frame->seq);
    uart_putc_raw(UART_ID, frame->len);
    for(int i = 0; i < frame->len; i++) {
        uart_putc_raw(UART_ID, frame->data[i]);
    }
    uart_putc_raw(UART_ID, frame->checksum);
}

bool recv_frame(frame_t* frame) {
    if(!uart_is_readable(UART_ID)) {
        return false;
    }
    uint8_t type = uart_getc(UART_ID);
    uint8_t seq = uart_getc(UART_ID);
    uint8_t len = uart_getc(UART_ID);
    uint8_t buf[3 + MAX_PAYLOAD];
    buf[0] = type;
    buf[1] = seq;
    buf[2] = len;

    for(uint8_t i = 0; i < len; i++) {
        buf[3 + i] = uart_getc(UART_ID);
        printf("Received byte %d: 0x%02X %c\n", i, buf[3 + i], buf[3 + i]);
    }
    uint8_t checksum = uart_getc(UART_ID);
    buf[3 + len] = checksum;
    if(!proto_decode(frame, buf, 3 + len + 1)) {
        printf("unable to decode frame\n");
        return false;
    }
    return true;
}

#endif // COMUNICATION_H
