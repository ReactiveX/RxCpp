#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("scope sample"){
    printf("//! [scope sample]\n");
    typedef rxcpp::resource<std::vector<int>> resource;
    auto resource_factory = [](){return resource(rxcpp::util::to_vector({1, 2, 3, 4, 5}));};
    auto observable_factory = [](resource res){return rxcpp::observable<>::iterate(res.get());};
    auto values = rxcpp::observable<>::scope(resource_factory, observable_factory);
    values.
        subscribe(
            [](int v){printf("OnNext: %d\n", v);},
            [](){printf("OnCompleted\n");});
    printf("//! [scope sample]\n");
}
