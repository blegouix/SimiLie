# SPDX-License-Identifier: GPL-3.0

include(GoogleTest)

add_executable(unit_tests_tensor tensor.cpp ../main.cpp)

target_link_libraries(unit_tests_tensor 
    PUBLIC
        GTest::gtest
        DDC::DDC
        sil::tensor
) 

gtest_discover_tests(unit_tests_tensor DISCOVERY_MODE PRE_TEST)
