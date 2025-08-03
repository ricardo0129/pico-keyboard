# Download Catch2 or use vendored version
add_subdirectory(../lib/catch2 Catch2)

# Add test file(s)
add_executable(tests
    test_mylib.cpp
)

target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
target_include_directories(tests PRIVATE ../include)

include(CTest)
include(Catch)
catch_discover_tests(tests)
