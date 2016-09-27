#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <iostream>
#include <thread>
#include <string>
std::string get_pid() {
    std::stringstream s;
    s << std::this_thread::get_id();
    return s.str();
}
