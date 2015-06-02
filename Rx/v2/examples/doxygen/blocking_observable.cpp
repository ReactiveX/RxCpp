#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("blocking first sample"){
    printf("//! [blocking first sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).as_blocking();
    auto first = values.first();
    printf("first = %d\n", first);
    printf("//! [blocking first sample]\n");
}

SCENARIO("blocking first empty sample"){
    printf("//! [blocking first empty sample]\n");
    auto values = rxcpp::observable<>::empty<int>().as_blocking();
    try {
        auto first = values.first();
        printf("first = %d\n", first);
    } catch (const std::exception& ex) {
        printf("Exception: %s\n", ex.what());
    }
    printf("//! [blocking first empty sample]\n");
}

SCENARIO("blocking last sample"){
    printf("//! [blocking last sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).as_blocking();
    auto last = values.last();
    printf("last = %d\n", last);
    printf("//! [blocking last sample]\n");
}

SCENARIO("blocking last empty sample"){
    printf("//! [blocking last empty sample]\n");
    auto values = rxcpp::observable<>::empty<int>().as_blocking();
    try {
        auto last = values.last();
        printf("last = %d\n", last);
    } catch (const std::exception& ex) {
        printf("Exception: %s\n", ex.what());
    }
    printf("//! [blocking last empty sample]\n");
}

SCENARIO("blocking count sample"){
    printf("//! [blocking count sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).as_blocking();
    auto count = values.count();
    printf("count = %d\n", count);
    printf("//! [blocking count sample]\n");
}

SCENARIO("blocking sum sample"){
    printf("//! [blocking sum sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).as_blocking();
    auto sum = values.sum();
    printf("sum = %d\n", sum);
    printf("//! [blocking sum sample]\n");
}

SCENARIO("blocking sum empty sample"){
    printf("//! [blocking sum empty sample]\n");
    auto values = rxcpp::observable<>::empty<int>().as_blocking();
    try {
        auto sum = values.sum();
        printf("sum = %d\n", sum);
    } catch (const std::exception& ex) {
        printf("Exception: %s\n", ex.what());
    }
    printf("//! [blocking sum empty sample]\n");
}

SCENARIO("blocking average sample"){
    printf("//! [blocking average sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).as_blocking();
    auto average = values.average();
    printf("average = %d\n", average);
    printf("//! [blocking average sample]\n");
}

SCENARIO("blocking average empty sample"){
    printf("//! [blocking average empty sample]\n");
    auto values = rxcpp::observable<>::empty<int>().as_blocking();
    try {
        auto average = values.average();
        printf("average = %d\n", average);
    } catch (const std::exception& ex) {
        printf("Exception: %s\n", ex.what());
    }
    printf("//! [blocking average empty sample]\n");
}
