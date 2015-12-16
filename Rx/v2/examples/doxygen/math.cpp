#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("first sample"){
    printf("//! [first sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).first();
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [first sample]\n");
}

SCENARIO("first empty sample"){
    printf("//! [first empty sample]\n");
    auto values = rxcpp::observable<>::empty<int>().first();
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](std::exception_ptr ep){
                try {std::rethrow_exception(ep);}
                catch (const rxcpp::empty_error& ex) {
                    printf("OnError: %s\n", ex.what());
                }
            },
            [](){printf("OnCompleted\n");});
    printf("//! [first empty sample]\n");
}

SCENARIO("last sample"){
    printf("//! [last sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).last();
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [last sample]\n");
}

SCENARIO("last empty sample"){
    printf("//! [last empty sample]\n");
    auto values = rxcpp::observable<>::empty<int>().last();
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](std::exception_ptr ep){
                try {std::rethrow_exception(ep);}
                catch (const rxcpp::empty_error& ex) {
                    printf("OnError: %s\n", ex.what());
                }
            },
            [](){printf("OnCompleted\n");});
    printf("//! [last empty sample]\n");
}

SCENARIO("count sample"){
    printf("//! [count sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).count();
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [count sample]\n");
}

SCENARIO("count error sample"){
    printf("//! [count error sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).
        concat(rxcpp::observable<>::error<int>(std::runtime_error("Error from source"))).
        count();
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](std::exception_ptr ep){
                try {std::rethrow_exception(ep);}
                catch (const std::runtime_error& ex) {
                    printf("OnError: %s\n", ex.what());
                }
            },
            [](){printf("OnCompleted\n");});
    printf("//! [count error sample]\n");
}

SCENARIO("sum sample"){
    printf("//! [sum sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).sum();
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [sum sample]\n");
}

SCENARIO("sum empty sample"){
    printf("//! [sum empty sample]\n");
    auto values = rxcpp::observable<>::empty<int>().sum();
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](std::exception_ptr ep){
                try {std::rethrow_exception(ep);}
                catch (const rxcpp::empty_error& ex) {
                    printf("OnError: %s\n", ex.what());
                }
            },
            [](){printf("OnCompleted\n");});
    printf("//! [sum empty sample]\n");
}

SCENARIO("sum error sample"){
    printf("//! [sum error sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).
        concat(rxcpp::observable<>::error<int>(std::runtime_error("Error from source"))).
        sum();
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](std::exception_ptr ep){
                try {std::rethrow_exception(ep);}
                catch (const std::runtime_error& ex) {
                    printf("OnError: %s\n", ex.what());
                }
            },
            [](){printf("OnCompleted\n");});
    printf("//! [sum error sample]\n");
}

SCENARIO("average sample"){
    printf("//! [average sample]\n");
    auto values = rxcpp::observable<>::range(1, 4).average();
    values.
        subscribe(
            [](double v){printf("OnNext: %lf\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [average sample]\n");
}

SCENARIO("average empty sample"){
    printf("//! [average empty sample]\n");
    auto values = rxcpp::observable<>::empty<int>().average();
    values.
        subscribe(
            [](double v){printf("OnNext: %lf\n", v);},
            [](std::exception_ptr ep){
                try {std::rethrow_exception(ep);}
                catch (const rxcpp::empty_error& ex) {
                    printf("OnError: %s\n", ex.what());
                }
            },
            [](){printf("OnCompleted\n");});
    printf("//! [average empty sample]\n");
}

SCENARIO("average error sample"){
    printf("//! [average error sample]\n");
    auto values = rxcpp::observable<>::range(1, 4).
        concat(rxcpp::observable<>::error<int>(std::runtime_error("Error from source"))).
        average();
    values.
        subscribe(
            [](double v){printf("OnNext: %lf\n", v);},
            [](std::exception_ptr ep){
                try {std::rethrow_exception(ep);}
                catch (const std::runtime_error& ex) {
                    printf("OnError: %s\n", ex.what());
                }
            },
            [](){printf("OnCompleted\n");});
    printf("//! [average error sample]\n");
}

SCENARIO("max sample"){
    printf("//! [max sample]\n");
    auto values = rxcpp::observable<>::range(1, 4).max();
    values.
        subscribe(
            [](double v){printf("OnNext: %lf\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [max sample]\n");
}

SCENARIO("max empty sample"){
    printf("//! [max empty sample]\n");
    auto values = rxcpp::observable<>::empty<int>().max();
    values.
        subscribe(
            [](double v){printf("OnNext: %lf\n", v);},
            [](std::exception_ptr ep){
                try {std::rethrow_exception(ep);}
                catch (const rxcpp::empty_error& ex) {
                    printf("OnError: %s\n", ex.what());
                }
            },
            [](){printf("OnCompleted\n");});
    printf("//! [max empty sample]\n");
}

SCENARIO("max error sample"){
    printf("//! [max error sample]\n");
    auto values = rxcpp::observable<>::range(1, 4).
        concat(rxcpp::observable<>::error<int>(std::runtime_error("Error from source"))).
            max();
    values.
        subscribe(
            [](double v){printf("OnNext: %lf\n", v);},
            [](std::exception_ptr ep){
                try {std::rethrow_exception(ep);}
                catch (const std::runtime_error& ex) {
                    printf("OnError: %s\n", ex.what());
                }
            },
            [](){printf("OnCompleted\n");});
    printf("//! [max error sample]\n");
}

SCENARIO("min sample"){
    printf("//! [min sample]\n");
    auto values = rxcpp::observable<>::range(1, 4).min();
    values.
        subscribe(
            [](double v){printf("OnNext: %lf\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [min sample]\n");
}

SCENARIO("min empty sample"){
    printf("//! [min empty sample]\n");
    auto values = rxcpp::observable<>::empty<int>().min();
    values.
        subscribe(
            [](double v){printf("OnNext: %lf\n", v);},
            [](std::exception_ptr ep){
                try {std::rethrow_exception(ep);}
                catch (const rxcpp::empty_error& ex) {
                    printf("OnError: %s\n", ex.what());
                }
            },
            [](){printf("OnCompleted\n");});
    printf("//! [min empty sample]\n");
}

SCENARIO("min error sample"){
    printf("//! [min error sample]\n");
    auto values = rxcpp::observable<>::range(1, 4).
        concat(rxcpp::observable<>::error<int>(std::runtime_error("Error from source"))).
            min();
    values.
        subscribe(
            [](double v){printf("OnNext: %lf\n", v);},
            [](std::exception_ptr ep){
                try {std::rethrow_exception(ep);}
                catch (const std::runtime_error& ex) {
                    printf("OnError: %s\n", ex.what());
                }
            },
            [](){printf("OnCompleted\n");});
    printf("//! [min error sample]\n");
}
