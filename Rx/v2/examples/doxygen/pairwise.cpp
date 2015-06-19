#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("pairwise sample"){
    printf("//! [pairwise sample]\n");
    auto values = rxcpp::observable<>::range(1, 5).pairwise();
    values.
        subscribe(
            [](std::tuple<int, int> v){printf("OnNext: %d, %d\n", std::get<0>(v), std::get<1>(v));},
            [](){printf("OnCompleted\n");});
    printf("//! [pairwise sample]\n");
}

SCENARIO("pairwise short sample"){
    printf("//! [pairwise short sample]\n");
    auto values = rxcpp::observable<>::just(1).pairwise();
    values.
        subscribe(
            [](std::tuple<int, int> v){printf("OnNext: %d, %d\n", std::get<0>(v), std::get<1>(v));},
            [](){printf("OnCompleted\n");});
    printf("//! [pairwise short sample]\n");
}

//std::string get_pid();
//
//SCENARIO("threaded flat_map sample"){
//    printf("//! [threaded flat_map sample]\n");
//    printf("[thread %s] Start task\n", get_pid().c_str());
//    auto values = rxcpp::observable<>::range(1, 3).
//        flat_map(
//            [](int v){
//                printf("[thread %s] Call CollectionSelector(v = %d)\n", get_pid().c_str(), v);
//                return
//                    rxcpp::observable<>::interval(std::chrono::steady_clock::now() + std::chrono::milliseconds(10 * v), std::chrono::milliseconds(50)).
//                    take(3);
//            },
//            [](int v_main, int v_sub){
//                printf("[thread %s] Call ResultSelector(v_main = %d, v_sub = %d)\n", get_pid().c_str(), v_main, v_sub);
//                return std::make_tuple(v_main, v_sub);
//            },
//            rxcpp::observe_on_new_thread());
//    values.
//        as_blocking().
//        subscribe(
//            [](std::tuple<int, long> v){printf("[thread %s] OnNext: %d - %d\n", get_pid().c_str(), std::get<0>(v), std::get<1>(v));},
//            [](){printf("[thread %s] OnCompleted\n", get_pid().c_str());});
//    printf("[thread %s] Finish task\n", get_pid().c_str());
//    printf("//! [threaded flat_map sample]\n");
//}
