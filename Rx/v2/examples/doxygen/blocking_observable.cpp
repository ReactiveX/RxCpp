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
    } catch (const rxcpp::empty_error& ex) {
        printf("Exception: %s\n", ex.what());
    }
    printf("//! [blocking first empty sample]\n");
}

SCENARIO("blocking first error sample"){
    printf("//! [blocking first error sample]\n");
    auto values = rxcpp::observable<>::error<int>(std::runtime_error("Error from source")).
        as_blocking();
    try {
        auto first = values.first();
        printf("first = %d\n", first);
    } catch (const std::exception& ex) {
        printf("Exception: %s\n", ex.what());
    }
    printf("//! [blocking first error sample]\n");
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
    } catch (const rxcpp::empty_error& ex) {
        printf("Exception: %s\n", ex.what());
    }
    printf("//! [blocking last empty sample]\n");
}

SCENARIO("blocking last error sample"){
    printf("//! [blocking last error sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).
        concat(rxcpp::observable<>::error<int>(std::runtime_error("Error from source"))).
        as_blocking();
    try {
        auto last = values.last();
        printf("last = %d\n", last);
    } catch (const std::exception& ex) {
        printf("Exception: %s\n", ex.what());
    }
    printf("//! [blocking last error sample]\n");
}

SCENARIO("blocking count sample"){
    printf("//! [blocking count sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).as_blocking();
    auto count = values.count();
    printf("count = %d\n", count);
    printf("//! [blocking count sample]\n");
}

SCENARIO("blocking count error sample"){
    printf("//! [blocking count error sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).
        concat(rxcpp::observable<>::error<int>(std::runtime_error("Error from source"))).
        as_blocking();
    try {
        auto count = values.count();
        printf("count = %d\n", count);
    } catch (const std::exception& ex) {
        printf("Exception: %s\n", ex.what());
    }
    printf("//! [blocking count error sample]\n");
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
    } catch (const rxcpp::empty_error& ex) {
        printf("Exception: %s\n", ex.what());
    }
    printf("//! [blocking sum empty sample]\n");
}

SCENARIO("blocking sum error sample"){
    printf("//! [blocking sum error sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).
        concat(rxcpp::observable<>::error<int>(std::runtime_error("Error from source"))).
        as_blocking();
    try {
        auto sum = values.sum();
        printf("sum = %d\n", sum);
    } catch (const std::exception& ex) {
        printf("Exception: %s\n", ex.what());
    }
    printf("//! [blocking sum error sample]\n");
}

SCENARIO("blocking average sample"){
    printf("//! [blocking average sample]\n");
    auto values = rxcpp::observable<>::range(1, 4).as_blocking();
    auto average = values.average();
    printf("average = %lf\n", average);
    printf("//! [blocking average sample]\n");
}

SCENARIO("blocking average empty sample"){
    printf("//! [blocking average empty sample]\n");
    auto values = rxcpp::observable<>::empty<int>().as_blocking();
    try {
        auto average = values.average();
        printf("average = %lf\n", average);
    } catch (const rxcpp::empty_error& ex) {
        printf("Exception: %s\n", ex.what());
    }
    printf("//! [blocking average empty sample]\n");
}

SCENARIO("blocking average error sample"){
    printf("//! [blocking average error sample]\n");
    auto values = rxcpp::observable<>::range(1, 4).
        concat(rxcpp::observable<>::error<int>(std::runtime_error("Error from source"))).
        as_blocking();
    try {
        auto average = values.average();
        printf("average = %lf\n", average);
    } catch (const std::exception& ex) {
        printf("Exception: %s\n", ex.what());
    }
    printf("//! [blocking average error sample]\n");
}

SCENARIO("blocking max sample"){
    printf("//! [blocking max sample]\n");
    auto values = rxcpp::observable<>::range(1, 4).as_blocking();
    auto max = values.max();
    printf("max = %d\n", max);
    printf("//! [blocking max sample]\n");
}

SCENARIO("blocking max empty sample"){
    printf("//! [blocking max empty sample]\n");
    auto values = rxcpp::observable<>::empty<int>().as_blocking();
    try {
        auto max = values.max();
        printf("max = %d\n", max);
    } catch (const rxcpp::empty_error& ex) {
        printf("Exception: %s\n", ex.what());
    }
    printf("//! [blocking max empty sample]\n");
}

SCENARIO("blocking max error sample"){
    printf("//! [blocking max error sample]\n");
    auto values = rxcpp::observable<>::range(1, 4).
        concat(rxcpp::observable<>::error<int>(std::runtime_error("Error from source"))).
        as_blocking();
    try {
        auto max = values.max();
        printf("max = %d\n", max);
    } catch (const std::exception& ex) {
        printf("Exception: %s\n", ex.what());
    }
    printf("//! [blocking max error sample]\n");
}

SCENARIO("blocking min sample"){
    printf("//! [blocking min sample]\n");
    auto values = rxcpp::observable<>::range(1, 4).as_blocking();
    auto min = values.min();
    printf("min = %d\n", min);
    printf("//! [blocking min sample]\n");
}

SCENARIO("blocking min empty sample"){
    printf("//! [blocking min empty sample]\n");
    auto values = rxcpp::observable<>::empty<int>().as_blocking();
    try {
        auto min = values.min();
        printf("min = %d\n", min);
    } catch (const rxcpp::empty_error& ex) {
        printf("Exception: %s\n", ex.what());
    }
    printf("//! [blocking min empty sample]\n");
}

SCENARIO("blocking min error sample"){
    printf("//! [blocking min error sample]\n");
    auto values = rxcpp::observable<>::range(1, 4).
        concat(rxcpp::observable<>::error<int>(std::runtime_error("Error from source"))).
        as_blocking();
    try {
        auto min = values.min();
        printf("min = %d\n", min);
    } catch (const std::exception& ex) {
        printf("Exception: %s\n", ex.what());
    }
    printf("//! [blocking min error sample]\n");
}
