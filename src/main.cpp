#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "pico_net/mylib.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include <unordered_map>

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

    return 0;
}
