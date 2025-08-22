#include <stdint.h>

static const uint32_t I2C_CHILD_ADDRESS = 0x17;
static const uint32_t I2C_BAUDRATE = 100000; // 100 kHz
const int32_t MAX_QUEUE_SIZE = 256;
const int32_t MAX_BUFFER_SIZE = 32;
// For this example, we run both the parent and child from the same board.
// You'll need to wire pin GP4 to GP6 (SDA), and pin GP5 to GP7 (SCL).
static const uint32_t I2C_CHILD_SDA_PIN = 4; // 4
static const uint32_t I2C_CHILD_SCL_PIN = 5; // 5
