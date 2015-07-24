#include "../test.h"

SCENARIO("defer stops on completion", "[defer][sources]"){
    GIVEN("a test cold observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<long> on;

        long invoked = 0;

        rxu::detail::maybe<rx::test::testable_observable<long>> xs;

        WHEN("deferred"){

            auto empty = rx::observable<>::empty<long>();
            auto just = rx::observable<>::just(42);
            auto one = rx::observable<>::from(42);
            auto error = rx::observable<>::error<long>(std::exception_ptr());
            auto runtimeerror = rx::observable<>::error<long>(std::runtime_error("runtime"));

            auto res = w.start(
                [&]() {
                    return rx::observable<>::defer(
                        [&](){
                            invoked++;
                            xs.reset(sc.make_cold_observable({
                                on.next(100, sc.clock()),
                                on.completed(200)
                            }));
                            return xs.get();
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output stops on completion"){
                auto required = rxu::to_vector({
                    on.next(300, 200L),
                    on.completed(400)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 400)
                });
                auto actual = xs.get().subscriptions();
                REQUIRE(required == actual);
            }

            THEN("defer was called until completed"){
                REQUIRE(1 == invoked);
            }
        }
    }
}
