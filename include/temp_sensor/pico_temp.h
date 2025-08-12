#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

const uint DHT_PIN = 15;
const uint MAX_TIMINGS = 85;

struct dht_reading {
    float humidity;
    float temp_celsius;
};

void read_from_dht(dht_reading *result);

