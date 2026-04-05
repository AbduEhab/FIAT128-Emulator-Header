cmake_minimum_required(VERSION 3.13)

add_executable(fiat128_bug_tests
    fiat128_bug_tests.cpp
)

target_include_directories(fiat128_bug_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
)

add_test(
    NAME fiat128_bug_tests
    COMMAND fiat128_bug_tests
)
