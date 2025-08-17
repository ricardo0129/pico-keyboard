#include "pico_net/mylib.h"

using namespace greeter;

Greeter::Greeter(std::string _name) : name(std::move(_name)) {}

std::string Greeter::greet(LanguageCode lang) const {
  switch (lang) {
    default:
    case LanguageCode::EN:
      return "Hello, {}!";
    case LanguageCode::DE:
      return "Hallo {}!";
    case LanguageCode::ES:
      return "¡Hola {}!";
    case LanguageCode::FR:
      return "Bonjour {}!";
  }
}
