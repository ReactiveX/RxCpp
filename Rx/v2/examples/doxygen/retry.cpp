#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("retry sample"){
    printf("//! [retry sample]\n");
    auto values = rxcpp::observable<>::from(1, 2).
        concat(rxcpp::observable<>::error<int>(std::runtime_error("Error from source"))).
        retry().
        take(5);
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [retry sample]\n");
}

SCENARIO("retry count sample"){
    printf("//! [retry count sample]\n");
    auto source = rxcpp::observable<>::from(1, 2).
        concat(rxcpp::observable<>::error<int>(std::runtime_error("Error from source")));
    auto values = source.retry(3);
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](std::exception_ptr ep){
                try {std::rethrow_exception(ep);}
                catch (const std::exception& ex) {
                    printf("OnError: %s\n", ex.what());
                }
            },
            [](){printf("OnCompleted\n");});
    printf("//! [retry count sample]\n");
}

//SCENARIO("retry hot sample"){
//    printf("//! [retry hot sample]\n");
//    auto values = rxcpp::observable<>::timer(std::chrono::milliseconds(10)).
//        concat(rxcpp::observable<>::error<long>(std::runtime_error("Error1 from source"))).
//        concat(rxcpp::observable<>::timer(std::chrono::milliseconds(10))).
//        concat(rxcpp::observable<>::error<long>(std::runtime_error("Error2 from source"))).
//        concat(rxcpp::observable<>::timer(std::chrono::milliseconds(10))).
//        concat(rxcpp::observable<>::error<long>(std::runtime_error("Error3 from source"))).
//        concat(rxcpp::observable<>::timer(std::chrono::milliseconds(10))).
//        concat(rxcpp::observable<>::error<long>(std::runtime_error("Error4 from source"))).
//        retry(3);
//    values.
//        subscribe(
//            [](long v){printf("OnNext: %d\n", v);},
//            [](std::exception_ptr ep){
//                try {std::rethrow_exception(ep);}
//                catch (const std::exception& ex) {
//                    printf("OnError: %s\n", ex.what());
//                }
//            },
//            [](){printf("OnCompleted\n");});
//    printf("//! [retry hot sample]\n");
//}
//
//SCENARIO("retry completed sample"){
//    printf("//! [retry completed sample <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<]\n");
//    auto source = rxcpp::observable<>::from(1, 2).
//        concat(rxcpp::observable<>::error<int>(std::runtime_error("Error from source"))).
//        publish();
//    auto values = source.retry();
//    //auto values = rxcpp::observable<>::timer(std::chrono::milliseconds(10)).
//    //    concat(rxcpp::observable<>::error<long>(std::runtime_error("Error1 from source"))).
//    //    concat(rxcpp::observable<>::timer(std::chrono::milliseconds(10))).
//    //    concat(rxcpp::observable<>::error<long>(std::runtime_error("Error2 from source"))).
//    //    concat(rxcpp::observable<>::timer(std::chrono::milliseconds(10))).
//    //    retry(3);
//    values.
//        subscribe(
//            [](long v){printf("OnNext: %d\n", v);},
//            [](std::exception_ptr ep){
//                try {std::rethrow_exception(ep);}
//                catch (const std::exception& ex) {
//                    printf("OnError: %s\n", ex.what());
//                }
//            },
//            [](){printf("OnCompleted\n");});
//    printf("//! [retry completed sample]\n");
//}
