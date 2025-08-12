#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include <unordered_map>

#include <string.h>
#include <time.h>

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "pico_net/mylib.h"

#include "pico_net/pico_net.h"



int main() {
    stdio_init_all();
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
