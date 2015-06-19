#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("as_dynamic sample"){
    printf("//! [as_dynamic sample]\n");
    auto o1 = rxcpp::observable<>::range(1, 3);
    auto o2 = rxcpp::observable<>::just(4);
    auto o3 = rxcpp::observable<>::empty<int>();
    auto values = o1.concat(o2, o3);
    printf("type of o1:                  %s\n", typeid(o1).name());
    printf("type of o1.as_dynamic():     %s\n", typeid(o1.as_dynamic()).name());
    printf("type of o2:                  %s\n", typeid(o2).name());
    printf("type of o2.as_dynamic():     %s\n", typeid(o2.as_dynamic()).name());
    printf("type of o3:                  %s\n", typeid(o3).name());
    printf("type of o3.as_dynamic():     %s\n", typeid(o3.as_dynamic()).name());
    printf("type of values:              %s\n", typeid(values).name());
    printf("type of values.as_dynamic(): %s\n", typeid(values.as_dynamic()).name());
    printf("//! [as_dynamic sample]\n");
}
