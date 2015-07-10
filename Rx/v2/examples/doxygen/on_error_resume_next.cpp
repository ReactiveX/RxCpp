#include "rxcpp/rx.hpp"
namespace rxu=rxcpp::util;

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("on_error_resume_next sample"){
    printf("//! [on_error_resume_next sample]\n");
    auto values = rxcpp::observable<>::range(1, 3).
        concat(rxcpp::observable<>::error<int>(std::runtime_error("Error from source"))).
        on_error_resume_next([](std::exception_ptr ep){
            printf("Resuming after: %s\n", rxu::what(ep).c_str());
            return rxcpp::observable<>::just(-1);
        });
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](std::exception_ptr ep){
                printf("OnError: %s\n", rxu::what(ep).c_str());
            },
            [](){printf("OnCompleted\n");});
    printf("//! [on_error_resume_next sample]\n");
}
