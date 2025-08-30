#ifndef PROTO_H
#define PROTO_H

#include <stdint.h>

const uint32_t MAX_PAYLOAD = 64;
#define TYPE_DATA 0x01
#define TYPE_ACK  0x02

struct frame_t {
    uint8_t type;
    uint8_t seq;
    uint8_t len;
    uint8_t data[MAX_PAYLOAD];
    uint8_t checksum;
};

bool proto_encode(frame_t* frame, uint8_t* data, uint8_t type, uint8_t len, uint8_t seq);
bool proto_decode(frame_t* frame, uint8_t* data, uint8_t size);
uint8_t proto_checksum(uint8_t* data, uint8_t len);


#endif // PROTO_H
