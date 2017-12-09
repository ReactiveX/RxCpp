#include "rxcpp/rx.hpp"
namespace rxu=rxcpp::util;

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("composite_exception sample"){
    printf("//! [composite_exception sample]\n");
    auto o1 = rxcpp::observable<>::error<int>(std::runtime_error("Error from source o1\n"));
    auto o2 = rxcpp::observable<>::error<int>(std::runtime_error("Error from source o2\n"));
    auto o3 = rxcpp::observable<>::timer(std::chrono::milliseconds(5)).map([](int) {return 3;});
    auto values = o1.merge_delay_error(o2, o3);
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](std::exception_ptr composite_e) {
                printf("OnError %s\n", rxu::what(composite_e).c_str());
                try { std::rethrow_exception(composite_e); }
                catch(rxcpp::composite_exception ce) {
                    for(std::exception_ptr particular_e : ce.exceptions) {

                        try{ std::rethrow_exception(particular_e); }
                        catch(std::runtime_error error) { printf(" *** %s\n", error.what()); }

                    }
                }
            },
            [](){printf("OnCompleted\n");}
        );
    printf("//! [composite_exception sample]\n");
}
