#include "../test.h"

SCENARIO("empty emits no items", "[empty][sources]"){
    GIVEN("an empty source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        WHEN("created"){

            auto res = w.start(
                []() {
                    return rx::observable<>::empty<int>()
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains the completion message"){
                auto required = rxu::to_vector({
                    on.completed(200)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("empty emits no items (rx::sources)", "[empty][sources]"){
    GIVEN("an empty source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        WHEN("created"){
            using namespace rx::sources;

            auto res = w.start(
                    []() {
                        return empty<int>()
                                // forget type to workaround lambda deduction bug on msvc 2013
                                .as_dynamic();
                    }
            );

            THEN("the output only contains the completion message"){
                auto required = rxu::to_vector({
                                                       on.completed(200)
                                               });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

        }
    }
}
