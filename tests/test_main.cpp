#define CATCH_CONFIG_MAIN
#include "catch_amalgamated.hpp"

#include "mylib.hpp"

TEST_CASE("Addition works", "[math]") {
    REQUIRE(add(3, 4) == 7);
}
