#include "../test.h"
#include <rxcpp/operators/rx-map.hpp>
#include <rxcpp/operators/rx-take_until.hpp>

SCENARIO("take_until trigger on_next", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(220, 3),
            on.next(230, 4),
            on.next(240, 5),
            on.completed(250)
        });

        auto ys = sc.make_hot_observable({
            on.next(150, 1),
            on.next(225, 99),
            on.completed(230)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [xs, ys]() {
                return xs
                    | rxo::take_until(ys)
                    // forget type to workaround lambda deduction bug on msvc 2013
                    | rxo::as_dynamic();
            }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
                    on.next(220, 3),
                    on.completed(225)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = ys.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, preempt some data next", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto l = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(220, 3),
            on.next(230, 4),
            on.next(240, 5),
            on.completed(250)
        });

        auto r = sc.make_hot_observable({
            on.next(150, 1),
            on.next(225, 99),
            on.completed(230)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r]() {
                return l
                    .take_until(r)
                    // forget type to workaround lambda deduction bug on msvc 2013
                    .as_dynamic();
            }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
                    on.next(220, 3),
                    on.completed(225)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = r.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, preempt some data error", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("take_until on_error from source");

        auto l = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(220, 3),
            on.next(230, 4),
            on.next(240, 5),
            on.completed(250)
        });

        auto r = sc.make_hot_observable({
            on.next(150, 1),
            on.error(225, ex)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r]() {
                return l
                    .take_until(r)
                    // forget type to workaround lambda deduction bug on msvc 2013
                    .as_dynamic();
            }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
                    on.next(220, 3),
                    on.error(225, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = r.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, no-preempt some data empty", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto l = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(220, 3),
            on.next(230, 4),
            on.next(240, 5),
            on.completed(250)
        });

        auto r = sc.make_hot_observable({
            on.next(150, 1),
            on.completed(225)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r]() {
                return l
                    .take_until(r)
                    // forget type to workaround lambda deduction bug on msvc 2013
                    .as_dynamic();
            }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
                    on.next(220, 3),
                    on.next(230, 4),
                    on.next(240, 5),
                    on.completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = r.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, no-preempt some data never", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto l = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(220, 3),
            on.next(230, 4),
            on.next(240, 5),
            on.completed(250)
        });

        auto r = sc.make_hot_observable({
            on.next(150, 1)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r]() {
                return l
                    .take_until(r)
                    // forget type to workaround lambda deduction bug on msvc 2013
                    .as_dynamic();
            }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(210, 2),
                    on.next(220, 3),
                    on.next(230, 4),
                    on.next(240, 5),
                    on.completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = r.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, preempt never next", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto l = sc.make_hot_observable({
            on.next(150, 1)
        });

        auto r = sc.make_hot_observable({
            on.next(150, 1),
            on.next(225, 2), //!
            on.completed(250)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r]() {
                return l
                    .take_until(r)
                    // forget type to workaround lambda deduction bug on msvc 2013
                    .as_dynamic();
            }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.completed(225)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = r.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, preempt never error", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("take_until on_error from source");

        auto l = sc.make_hot_observable({
            on.next(150, 1)
        });

        auto r = sc.make_hot_observable({
            on.next(150, 1),
            on.error(225, ex)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r]() {
                return l
                    .take_until(r)
                    // forget type to workaround lambda deduction bug on msvc 2013
                    .as_dynamic();
            }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.error(225, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = r.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, no-preempt never empty", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto l = sc.make_hot_observable({
            on.next(150, 1)
        });

        auto r = sc.make_hot_observable({
            on.next(150, 1),
            on.completed(225)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r]() {
                return l
                    .take_until(r)
                    // forget type to workaround lambda deduction bug on msvc 2013
                    .as_dynamic();
            }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = std::vector<rxsc::test::messages<int>::recorded_type>();
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 1000 /* can't dispose prematurely, could be in flight to dispatch OnError */)
                });
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = r.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, no-preempt never never", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto l = sc.make_hot_observable({
            on.next(150, 1)
        });

        auto r = sc.make_hot_observable({
            on.next(150, 1)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r]() {
                return l
                    .take_until(r)
                    // forget type to workaround lambda deduction bug on msvc 2013
                    .as_dynamic();
            }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = std::vector<rxsc::test::messages<int>::recorded_type>();
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 1000)
                });
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 1000)
                });
                auto actual = r.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, preempt before first produced", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto l = sc.make_hot_observable({
            on.next(150, 1),
            on.next(230, 2),
            on.completed(240)
        });

        auto r = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2), //!
            on.completed(220)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r]() {
                return l
                    .take_until(r)
                    // forget type to workaround lambda deduction bug on msvc 2013
                    .as_dynamic();
            }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.completed(210)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 210)
                });
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 210)
                });
                auto actual = r.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, preempt before first produced, remain silent and proper unsubscribed", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        bool sourceNotDisposed = false;

        auto l = sc.make_hot_observable({
            on.next(150, 1),
            on.error(215, std::runtime_error("error in unsubscribed stream")), // should not come
            on.completed(240)
        });

        auto r = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2), //!
            on.completed(220)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r, &sourceNotDisposed]() {
                return l
                    .map([&sourceNotDisposed](int v){sourceNotDisposed = true; return v; })
                    .take_until(r)
                    // forget type to workaround lambda deduction bug on msvc 2013
                    .as_dynamic();
            }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.completed(210)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("signal disposed"){
                auto required = false;
                auto actual = sourceNotDisposed;
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, no-preempt after last produced, proper unsubscribe signal", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        bool signalNotDisposed = false;

        auto l = sc.make_hot_observable({
            on.next(150, 1),
            on.next(230, 2),
            on.completed(240)
        });

        auto r = sc.make_hot_observable({
            on.next(150, 1),
            on.next(250, 2),
            on.completed(260)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r, &signalNotDisposed]() {
                return l
                    .take_until(r
                    .map([&signalNotDisposed](int v){signalNotDisposed = true; return v; }))
                    // forget type to workaround lambda deduction bug on msvc 2013
                    .as_dynamic();
            }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(230, 2),
                    on.completed(240)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("signal disposed"){
                auto required = false;
                auto actual = signalNotDisposed;
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until, error some", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("take_until on_error from source");

        auto l = sc.make_hot_observable({
            on.next(150, 1),
            on.error(225, ex)
        });

        auto r = sc.make_hot_observable({
            on.next(150, 1),
            on.next(240, 2)
        });

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start(
                [l, r]() {
                return l
                    .take_until(r)
                    // forget type to workaround lambda deduction bug on msvc 2013
                    .as_dynamic();
            }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.error(225, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
                });
                auto actual = r.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take_until trigger on time point", "[take_until][take][operators]"){
    GIVEN("a source and a time point"){
        auto sc = rxsc::make_test();
        auto so = rx::synchronize_in_one_worker(sc);
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto xs = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.next(220, 3),
            on.next(230, 4),
            on.next(240, 5),
            on.completed(250)
        });

        auto t = sc.to_time_point(225);

        WHEN("invoked with a time point"){

            auto res = w.start(
                [&]() {
                return xs
                    | rxo::take_until(t, so)
                    // forget type to workaround lambda deduction bug on msvc 2013
                    | rxo::as_dynamic();
            }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = rxu::to_vector({
                    on.next(211, 2),
                    on.next(221, 3),
                    on.completed(226)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 226)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}
