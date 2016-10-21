#include "../test.h"
#include "rxcpp/operators/rx-with_latest_from.hpp"

SCENARIO("with_latest_from interleaved with tail", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto o1 = sc.make_hot_observable({
            on.next(150, 1),
            on.next(215, 2),
            on.next(225, 4),
            on.completed(230)
        });

        auto o2 = sc.make_hot_observable({
            on.next(150, 1),
            on.next(220, 3),
            on.next(230, 5),
            on.next(235, 6),
            on.next(240, 7),
            on.completed(250)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return o2
                        .with_latest_from(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            o1
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains combined ints"){
                auto required = rxu::to_vector({
                    on.next(220, 2 + 3),
                    on.next(230, 4 + 5),
                    on.next(235, 4 + 6),
                    on.next(240, 4 + 7),
                    on.completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the o1"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 230)
                });
                auto actual = o1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the o2"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = o2.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("with_latest_from consecutive", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto o1 = sc.make_hot_observable({
            on.next(150, 1),
            on.next(215, 2),
            on.next(225, 4),
            on.completed(230)
        });

        auto o2 = sc.make_hot_observable({
            on.next(150, 1),
            on.next(235, 6),
            on.next(240, 7),
            on.completed(250)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return o2
                        .with_latest_from(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            o1
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains combined ints"){
                auto required = rxu::to_vector({
                    on.next(235, 4 + 6),
                    on.next(240, 4 + 7),
                    on.completed(250)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the o1"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 230)
                });
                auto actual = o1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the o2"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 250)
                });
                auto actual = o2.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("with_latest_from consecutive ends with error left", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("with_latest_from on_error from source");

        auto o1 = sc.make_hot_observable({
            on.next(150, 1),
            on.next(215, 2),
            on.next(225, 4),
            on.error(230, ex)
        });

        auto o2 = sc.make_hot_observable({
            on.next(150, 1),
            on.next(235, 6),
            on.next(240, 7),
            on.completed(250)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return o2
                        .with_latest_from(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            o1
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only an error"){
                auto required = rxu::to_vector({
                    on.error(230, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the o1"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 230)
                });
                auto actual = o1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the o2"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 230)
                });
                auto actual = o2.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("with_latest_from consecutive ends with error right", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("with_latest_from on_error from source");

        auto o1 = sc.make_hot_observable({
            on.next(150, 1),
            on.next(215, 2),
            on.next(225, 4),
            on.completed(250)
        });

        auto o2 = sc.make_hot_observable({
            on.next(150, 1),
            on.next(235, 6),
            on.next(240, 7),
            on.error(245, ex)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return o2
                        .with_latest_from(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            o1
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains combined ints followed by an error"){
                auto required = rxu::to_vector({
                    on.next(235, 4 + 6),
                    on.next(240, 4 + 7),
                    on.error(245, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the o1"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 245)
                });
                auto actual = o1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the o2"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 245)
                });
                auto actual = o2.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("with_latest_from never N", "[with_latest_from][join][operators]"){
    GIVEN("N never completed hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        const int N = 4;

        std::vector<rxcpp::test::testable_observable<int>> n;
        for (int i = 0; i < N; ++i) {
            n.push_back(
                sc.make_hot_observable({
                    on.next(150, 1)
                })
            );
        }

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return n[0]
                        .with_latest_from(
                            [](int v0, int v1, int v2, int v3){
                                return v0 + v1 + v2 + v3;
                            },
                            n[1], n[2], n[3]
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output is empty"){
                auto required = std::vector<rxsc::test::messages<int>::recorded_type>();
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to each observable"){

                std::for_each(n.begin(), n.end(), [&](rxcpp::test::testable_observable<int> &s){
                    auto required = rxu::to_vector({
                        on.subscribe(200, 1000)
                    });
                    auto actual = s.subscriptions();
                    REQUIRE(required == actual);
                });
            }
        }
    }
}

SCENARIO("with_latest_from empty N", "[with_latest_from][join][operators]"){
    GIVEN("N empty hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        const int N = 4;

        std::vector<rxcpp::test::testable_observable<int>> e;
        for (int i = 0; i < N; ++i) {
            e.push_back(
                sc.make_hot_observable({
                    on.next(150, 1),
                    on.completed(210 + 10 * i)
                })
            );
        }

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return e[0]
                        .with_latest_from(
                            [](int v0, int v1, int v2, int v3){
                                return v0 + v1 + v2 + v3;
                            },
                            e[1], e[2], e[3]
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only complete message"){
                auto required = rxu::to_vector({
                    on.completed(200 + 10 * N)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to each observable"){

                int i = 0;
                std::for_each(e.begin(), e.end(), [&](rxcpp::test::testable_observable<int> &s){
                    auto required = rxu::to_vector({
                        on.subscribe(200, 200 + 10 * ++i)
                    });
                    auto actual = s.subscriptions();
                    REQUIRE(required == actual);
                });
            }
        }
    }
}

SCENARIO("with_latest_from never/empty", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto n = sc.make_hot_observable({
            on.next(150, 1)
        });

        auto e = sc.make_hot_observable({
            on.next(150, 1),
            on.completed(210)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return n
                        .with_latest_from(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            e
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output is empty"){
                auto required = std::vector<rxsc::test::messages<int>::recorded_type>();
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the n"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 1000)
                });
                auto actual = n.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the e"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 210)
                });
                auto actual = e.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("with_latest_from empty/never", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto e = sc.make_hot_observable({
            on.next(150, 1),
            on.completed(210)
        });

        auto n = sc.make_hot_observable({
            on.next(150, 1)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return e
                        .with_latest_from(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            n
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output is empty"){
                auto required = std::vector<rxsc::test::messages<int>::recorded_type>();
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the e"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 210)
                });
                auto actual = e.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the n"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 1000)
                });
                auto actual = n.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("with_latest_from empty/return", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto e = sc.make_hot_observable({
            on.next(150, 1),
            on.completed(210)
        });

        auto o = sc.make_hot_observable({
            on.next(150, 1),
            on.next(215, 2),
            on.completed(220)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return e
                        .with_latest_from(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            o
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only complete message"){
                auto required = rxu::to_vector({
                    on.completed(220)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the e"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 210)
                });
                auto actual = e.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the o"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = o.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("with_latest_from return/empty", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto o = sc.make_hot_observable({
            on.next(150, 1),
            on.next(215, 2),
            on.completed(220)
        });

        auto e = sc.make_hot_observable({
            on.next(150, 1),
            on.completed(210)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return o
                        .with_latest_from(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            e
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only complete message"){
                auto required = rxu::to_vector({
                    on.completed(220)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the o"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = o.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the e"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 210)
                });
                auto actual = e.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("with_latest_from never/return", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto n = sc.make_hot_observable({
            on.next(150, 1)
        });

        auto o = sc.make_hot_observable({
            on.next(150, 1),
            on.next(215, 2),
            on.completed(220)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return n
                        .with_latest_from(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            o
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output is empty"){
                auto required = std::vector<rxsc::test::messages<int>::recorded_type>();
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the n"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 1000)
                });
                auto actual = n.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the o"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = o.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("with_latest_from return/never", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto o = sc.make_hot_observable({
            on.next(150, 1),
            on.next(215, 2),
            on.completed(220)
        });

        auto n = sc.make_hot_observable({
            on.next(150, 1)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return o
                        .with_latest_from(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            n
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output is empty"){
                auto required = std::vector<rxsc::test::messages<int>::recorded_type>();
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the n"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 1000)
                });
                auto actual = n.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the o"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = o.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}


SCENARIO("with_latest_from return/return", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto o1 = sc.make_hot_observable({
            on.next(150, 1),
            on.next(215, 2),
            on.completed(230)
        });

        auto o2 = sc.make_hot_observable({
            on.next(150, 1),
            on.next(220, 3),
            on.completed(240)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return o1
                        .with_latest_from(
                            [](int v2, int v1){
                             return v2 + v1;
                            },
                            o2
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains combined ints"){
                auto required = rxu::to_vector({
                    on.completed(240)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the o1"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 230)
                });
                auto actual = o1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the o2"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 240)
                });
                auto actual = o2.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("with_latest_from empty/error", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("with_latest_from on_error from source");

        auto emp = sc.make_hot_observable({
            on.next(150, 1),
            on.completed(230)
        });

        auto err = sc.make_hot_observable({
            on.next(150, 1),
            on.error(220, ex)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return emp
                        .with_latest_from(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            err
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only error message"){
                auto required = rxu::to_vector({
                    on.error(220, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the emp"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = emp.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the err"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = err.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("with_latest_from error/empty", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("with_latest_from on_error from source");

        auto err = sc.make_hot_observable({
            on.next(150, 1),
            on.error(220, ex)
        });

        auto emp = sc.make_hot_observable({
            on.next(150, 1),
            on.completed(230)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return err
                        .with_latest_from(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            emp
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only error message"){
                auto required = rxu::to_vector({
                    on.error(220, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the emp"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = emp.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the err"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = err.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("with_latest_from return/error", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("with_latest_from on_error from source");

        auto o = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.completed(230)
        });

        auto err = sc.make_hot_observable({
            on.next(150, 1),
            on.error(220, ex)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return o
                        .with_latest_from(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            err
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only error message"){
                auto required = rxu::to_vector({
                    on.error(220, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ret"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = o.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the err"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = err.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("with_latest_from error/return", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("with_latest_from on_error from source");

        auto err = sc.make_hot_observable({
            on.next(150, 1),
            on.error(220, ex)
        });

        auto ret = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.completed(230)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return err
                        .with_latest_from(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            ret
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only error message"){
                auto required = rxu::to_vector({
                    on.error(220, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ret"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = ret.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the err"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = err.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("with_latest_from error/error", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex1("with_latest_from on_error from source 1");
        std::runtime_error ex2("with_latest_from on_error from source 2");

        auto err1 = sc.make_hot_observable({
            on.next(150, 1),
            on.error(220, ex1)
        });

        auto err2 = sc.make_hot_observable({
            on.next(150, 1),
            on.error(230, ex2)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return err1
                        .with_latest_from(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            err2
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only error message"){
                auto required = rxu::to_vector({
                    on.error(220, ex1)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the err1"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = err1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the err2"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = err2.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("with_latest_from next+error/error", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex1("with_latest_from on_error from source 1");
        std::runtime_error ex2("with_latest_from on_error from source 2");

        auto err1 = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.error(220, ex1)
        });

        auto err2 = sc.make_hot_observable({
            on.next(150, 1),
            on.error(230, ex2)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return err1
                        .with_latest_from(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            err2
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only error message"){
                auto required = rxu::to_vector({
                    on.error(220, ex1)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the err1"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = err1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the err2"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = err2.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("with_latest_from error/next+error", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex1("with_latest_from on_error from source 1");
        std::runtime_error ex2("with_latest_from on_error from source 2");

        auto err1 = sc.make_hot_observable({
            on.next(150, 1),
            on.error(230, ex1)
        });

        auto err2 = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.error(220, ex2)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return err1
                        .with_latest_from(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            err2
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only error message"){
                auto required = rxu::to_vector({
                    on.error(220, ex2)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the err1"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = err1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the err2"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = err2.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("with_latest_from never/error", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("with_latest_from on_error from source");

        auto n = sc.make_hot_observable({
            on.next(150, 1)
        });

        auto err = sc.make_hot_observable({
            on.next(150, 1),
            on.error(220, ex)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return n
                        .with_latest_from(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            err
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only error message"){
                auto required = rxu::to_vector({
                    on.error(220, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the n"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = n.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the err"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = err.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("with_latest_from error/never", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("with_latest_from on_error from source");

        auto err = sc.make_hot_observable({
            on.next(150, 1),
            on.error(220, ex)
        });

        auto n = sc.make_hot_observable({
            on.next(150, 1)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return err
                        .with_latest_from(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            n
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only error message"){
                auto required = rxu::to_vector({
                    on.error(220, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the n"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = n.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the err"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = err.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("with_latest_from error after completed left", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("with_latest_from on_error from source");

        auto ret = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.completed(215)
        });

        auto err = sc.make_hot_observable({
            on.next(150, 1),
            on.error(220, ex)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return ret
                        .with_latest_from(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            err
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only error message"){
                auto required = rxu::to_vector({
                    on.error(220, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ret"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 215)
                });
                auto actual = ret.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the err"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = err.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("with_latest_from error after completed right", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("with_latest_from on_error from source");

        auto err = sc.make_hot_observable({
            on.next(150, 1),
            on.error(220, ex)
        });

        auto ret = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.completed(215)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return err
                        .with_latest_from(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            ret
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only error message"){
                auto required = rxu::to_vector({
                    on.error(220, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the ret"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 215)
                });
                auto actual = ret.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the err"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = err.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("with_latest_from selector throws", "[with_latest_from][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("with_latest_from on_error from source");

        auto o1 = sc.make_hot_observable({
            on.next(150, 1),
            on.next(215, 2),
            on.completed(230)
        });

        auto o2 = sc.make_hot_observable({
            on.next(150, 1),
            on.next(220, 3),
            on.completed(240)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return o2
                        .with_latest_from(
                            [&ex](int, int) -> int {
                                throw ex;
                            },
                            o1
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only error"){
                auto required = rxu::to_vector({
                    on.error(220, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the o1"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = o1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the o2"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 220)
                });
                auto actual = o2.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("with_latest_from selector throws N", "[with_latest_from][join][operators]"){
    GIVEN("N hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        const int N = 4;

        std::runtime_error ex("with_latest_from on_error from source");

        std::vector<rxcpp::test::testable_observable<int>> e;
        for (int i = 0; i < N; ++i) {
            e.push_back(
                sc.make_hot_observable({
                    on.next(210 + 10 * i, 1),
                    on.completed(500)
                })
            );
        }

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return e[3]
                        .with_latest_from(
                            [&ex](int, int, int, int) -> int {
                                throw ex;
                            },
                            e[0], e[1], e[2]
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only error"){
                auto required = rxu::to_vector({
                    on.error(200 + 10 * N, ex)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to each observable"){

                std::for_each(e.begin(), e.end(), [&](rxcpp::test::testable_observable<int> &s){
                    auto required = rxu::to_vector({
                        on.subscribe(200, 200 + 10 * N)
                    });
                    auto actual = s.subscriptions();
                    REQUIRE(required == actual);
                });
            }
        }
    }
}

SCENARIO("with_latest_from typical N", "[with_latest_from][join][operators]"){
    GIVEN("N hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        const int N = 4;

        std::vector<rxcpp::test::testable_observable<int>> o;
        for (int i = 0; i < N; ++i) {
            o.push_back(
                sc.make_hot_observable({
                    on.next(150, 1),
                    on.next(210 + 10 * i, i + 1),
                    on.next(410 + 10 * i, i + N + 1),
                    on.completed(800)
                })
            );
        }

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return o[3]
                        .with_latest_from(
                            [](int v0, int v1, int v2, int v3) {
                                return v0 + v1 + v2 + v3;
                            },
                            o[0], o[1], o[2]
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains combined ints"){
                auto required = rxu::to_vector({
                    on.next(200 + 10 * N, N * (N + 1) / 2),
                    on.next(410 + 10 * (N - 1), (N - 1) * N / 2 + N + N * N)
                });
                required.push_back(on.completed(800));
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to each observable"){

                std::for_each(o.begin(), o.end(), [&](rxcpp::test::testable_observable<int> &s){
                    auto required = rxu::to_vector({
                        on.subscribe(200, 800)
                    });
                    auto actual = s.subscriptions();
                    REQUIRE(required == actual);
                });
            }
        }
    }
}
