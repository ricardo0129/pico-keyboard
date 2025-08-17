#include "temp_sensor/pico_temp.h"


int wait_for_switch(uint pin, uint previous) {
    int count = 0;
    while(gpio_get(pin) == previous) {
        count++;
        sleep_us(1);
        if (count == 255) return -1;
    }
    return count;
}
void read_from_dht(dht_reading *result) {
    int data[5] = {0, 0, 0, 0, 0};

    gpio_set_dir(DHT_PIN, GPIO_OUT);
    gpio_put(DHT_PIN, 0);
    sleep_ms(18);
    gpio_put(DHT_PIN, 1);
    sleep_us(20);
    gpio_set_dir(DHT_PIN, GPIO_IN);

    const int N = 40 * 2;
    int last = 0;
    int j = 0;
    int bit_length[N] = {};
    for(int i = 0; i < 2; i++) {
        wait_for_switch(DHT_PIN, last);
        last = gpio_get(DHT_PIN);
    }
    for(int i = 0; i < N; i++) {
        bit_length[i] = wait_for_switch(DHT_PIN, last);
        last = gpio_get(DHT_PIN);
        if (bit_length[i] < 0) {
            printf("Timeout waiting for switch\n");
            return;
        }
        if(i%2 == 1) {
            uint bit = bit_length[i] > 35;
            data[j / 8] <<= 1;
            data[j / 8] |= bit;
            j++;
        }
    }
    int checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
    if( checksum == data[4]) {
        /*
        result->humidity = (float) ((data[0] << 8) + data[1]) / 10;
        if (result->humidity > 100) {
            result->humidity = data[0];
        }
        */
        result->temp_celsius = (float) ((data[2] & 0x7F) << 8 | data[3]) / 10.0;
        printf("Temp: %f\n", result->temp_celsius);
    } else {
        printf("Bad data\n");
    }
}

