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

//SCENARIO("first empty sample"){
//    printf("//! [first empty sample]\n");
//    auto values = rxcpp::observable<>::empty<int>().first();
//    values.
//        subscribe(
//            [](int v){printf("OnNext: %d\n", v);},
//            [](std::exception_ptr ep){
//                try {std::rethrow_exception(ep);}
//                catch (const std::exception& ex) {
//                    printf("OnError: %s\n", ex.what());
//                }
//                catch (...) {
//                    printf("OnError:\n");
//                }
//            },
//            [](){printf("OnCompleted\n");});
//    printf("//! [first empty sample]\n");
//}

SCENARIO("last sample"){
    printf("//! [last sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).last();
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [last sample]\n");
}

//SCENARIO("last empty sample"){
//    printf("//! [last empty sample]\n");
//    auto values = rxcpp::observable<>::empty<int>().last();
//    values.
//        subscribe(
//            [](int v){printf("OnNext: %d\n", v);},
//            [](std::exception_ptr ep){
//                try {std::rethrow_exception(ep);}
//                catch (const std::exception& ex) {
//                    printf("OnError: %s\n", ex.what());
//                }
//            },
//            [](){printf("OnCompleted\n");});
//    printf("//! [last empty sample]\n");
//}

SCENARIO("count sample"){
    printf("//! [count sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).count();
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [count sample]\n");
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

SCENARIO("average sample"){
    printf("//! [average sample]\n");
    auto values = rxcpp::observable<>::range(1, 4).average();
    values.
        subscribe(
            [](double v){printf("OnNext: %lf\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [average sample]\n");
}
