#include "temp_sensor/pico_temp.h"


int wait_for_switch(uint pin, uint previous) {
    int count = 0;
    while(gpio_get(pin) == previous) {
        count++;
        sleep_us(1);
        if (count > 255) return -1;
    }
    return count;
}
void read_from_dht(dht_reading *result) {
    int data[5] = {0, 0, 0, 0, 0};
    uint j = 0;

    // Send start signal to DHT sensor
    gpio_set_dir(DHT_PIN, GPIO_OUT);
    gpio_put(DHT_PIN, 0);
    //wait at least 18ms
    sleep_ms(20);
    uint last = 0;
    for(int i = 0; i < 3; i++) {
        wait_for_switch(DHT_PIN, last);
        last = gpio_get(DHT_PIN);
    }
    // Read 40 bits of data from DHT sensor
    for(int i = 0; i < 40; i++) {
        wait_for_switch(DHT_PIN, 0);
        int count = wait_for_switch(DHT_PIN, 1);
        if (count < 0) {
            return;
        }
        uint signal = count > 20;
        data[i / 8] <<= 1;
        data[i / 8] |= signal;
    }
    // Verify checksum
    uint checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        return;
    }
    else {
        printf("Checksum valid\n");
    }
}

