#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("group_by sample"){
    printf("//! [group_by sample]\n");
    auto values = rxcpp::observable<>::range(0, 8).
        group_by(
            [](int v){return v % 3;},
            [](int v){return 10 * v;});
    values.
        subscribe(
            [](rxcpp::grouped_observable<int, int> g){
                auto key = g.get_key();
                printf("OnNext: key = %d\n", key);
                g.subscribe(
                    [key](int v){printf("[key %d] OnNext: %d\n", key, v);},
                    [key](){printf("[key %d] OnCompleted\n", key);});
            },
            [](){printf("OnCompleted\n");});
    printf("//! [group_by sample]\n");
}

//! [group_by full intro]
bool less(int v1, int v2){
    return v1 < v2;
}
//! [group_by full intro]

SCENARIO("group_by full sample"){
    printf("//! [group_by full sample]\n");
    auto data = rxcpp::observable<>::range(0, 8).
        map([](int v){
            std::stringstream s;
            s << "Value " << v;
            return std::make_pair(v % 3, s.str());
        });
    auto values = data.group_by(
            [](std::pair<int, std::string> v){return v.first;},
            [](std::pair<int, std::string> v){return v.second;},
            less);
    values.
        subscribe(
            [](rxcpp::grouped_observable<int, std::string> g){
                auto key = g.get_key();
                printf("OnNext: key = %d\n", key);
                g.subscribe(
                    [key](const std::string& v){printf("[key %d] OnNext: %s\n", key, v.c_str());},
                    [key](){printf("[key %d] OnCompleted\n", key);});
            },
            [](){printf("OnCompleted\n");});
    printf("//! [group_by full sample]\n");
}
