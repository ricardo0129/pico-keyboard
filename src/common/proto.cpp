#include "common/proto.h"

bool proto_encode(frame_t *frame, uint8_t *data, uint8_t type, uint8_t len,
                  uint8_t seq) {
  if (len > MAX_PAYLOAD) {
    // Payload too large
    return false;
  }
  frame->type = type;
  frame->seq = seq;
  frame->len = len;
  for (int i = 0; i < len; i++) {
    frame->data[i] = data[i];
  }
  frame->checksum = proto_checksum(data, len);
  return true;
}

bool proto_decode(frame_t *frame, uint8_t *data, uint8_t size) {
  if (size < 4) {
    // Not enough data for type, seq, len, checksum
    return false;
  }
  uint8_t len = data[2];
  for (uint8_t i = 0; i < len; i++) {
    frame->data[i] = data[3 + i];
  }
  frame->type = data[0];
  frame->seq = data[1];
  frame->len = len;
  frame->checksum = data[3 + len];
  return frame->checksum == proto_checksum(frame->data, frame->len);
}

uint8_t proto_checksum(uint8_t *data, uint8_t len) {
  uint8_t checksum = 0;
  for (int i = 0; i < len; i++) {
    checksum ^= data[i];
  }
  return checksum;
}
