#include <unordered_map>
#include <string.h>
#include <time.h>

#include "pico_net/mylib.h"
#include "pico_net/pico_net.h"
#include "pico/stdlib.h"
#include "temp_sensor/pico_temp.h"


void pico_net::run_tcp_client_test(void) {
    TCP_CLIENT_T *state = tcp_client_init();
    if (!state) {
        return;
    }
    if (!tcp_client_open(state)) {
        tcp_result(state, -1);
        return;
    }
    while (!state->complete) {
        /*
        dht_reading reading;
        read_from_dht(&reading);
        float fahrenheit = (reading.temp_celsius * 9 / 5) + 32;
        printf("Humidity = %.1f%%, Temperature = %.1fC (%.1fF)\n",
               reading.humidity, reading.temp_celsius, fahrenheit);
        */
        printf("Polling...\n");
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
        //sleep_ms(2000);
    }
    free(state);
}

int main() {
    stdio_init_all();

    gpio_init(DHT_PIN);

    const std::unordered_map<std::string, greeter::LanguageCode> languages{
        {"en", greeter::LanguageCode::EN},
        {"de", greeter::LanguageCode::DE},
        {"es", greeter::LanguageCode::ES},
        {"fr", greeter::LanguageCode::FR},
    };
    std::string language = "en";

    auto langIt = languages.find(language);

    greeter::Greeter greeter("RICKY");
    printf("{}\n", greeter.greet(langIt->second));

    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        return 1;
    } 
    else {
        printf("Connected.\n");
    }
    pico_net::run_tcp_client_test();
    cyw43_arch_deinit();
    return 0;
}
