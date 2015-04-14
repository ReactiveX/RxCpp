#include "rxcpp/rx.hpp"
namespace rxu=rxcpp::util;
namespace rxsc=rxcpp::schedulers;

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("zip return/error", "[zip][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("zip on_error from source");

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
                        .zip(
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

SCENARIO("zip error/return", "[zip][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("zip on_error from source");

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
                        .zip(
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

SCENARIO("zip left completes first", "[zip][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto o1 = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.completed(220)
        });

        auto o2 = sc.make_hot_observable({
            on.next(150, 1),
            on.next(215, 4),
            on.completed(225)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return o2
                        .zip(
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
                    on.next(215, 2 + 4),
                    on.completed(225)
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
                    on.subscribe(200, 225)
                });
                auto actual = o2.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("zip right completes first", "[zip][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto o1 = sc.make_hot_observable({
            on.next(150, 1),
            on.next(215, 4),
            on.completed(225)
        });

        auto o2 = sc.make_hot_observable({
            on.next(150, 1),
            on.next(210, 2),
            on.completed(220)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return o2
                        .zip(
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
                    on.next(215, 2 + 4),
                    on.completed(225)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the o1"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 225)
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

SCENARIO("zip selector throws", "[zip][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("zip on_error from source");

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
                        .zip(
                            [&ex](int, int) -> int {
                                throw ex;
                            },
                            o2
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

SCENARIO("zip selector throws N", "[zip][join][operators]"){
    GIVEN("N hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        const int N = 16;

        std::runtime_error ex("zip on_error from source");

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
                    return e[0]
                        .zip(
                            [&ex](int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int) -> int {
                                throw ex;
                            },
                            e[1], e[2], e[3], e[4], e[5], e[6], e[7], e[8], e[9], e[10], e[11], e[12], e[13], e[14], e[15]
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

SCENARIO("zip typical N", "[zip][join][operators]"){
    GIVEN("N hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        const int N = 16;

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
                    return o[0]
                        .zip(
                            [](int v0, int v1, int v2, int v3, int v4, int v5, int v6, int v7, int v8, int v9, int v10, int v11, int v12, int v13, int v14, int v15) {
                                return v0 + v1 + v2 + v3 + v4 + v5 + v6 + v7 + v8 + v9 + v10 + v11 + v12 + v13 + v14 + v15;
                            },
                            o[1], o[2], o[3], o[4], o[5], o[6], o[7], o[8], o[9], o[10], o[11], o[12], o[13], o[14], o[15]
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains combined ints"){
                auto required = rxu::to_vector({
                    on.next(200 + 10 * N, N * (N + 1) / 2),
                    on.next(400 + 10 * N, N * (3 * N + 1) / 2)
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

SCENARIO("zip interleaved with tail", "[zip][join][operators]"){
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
                        .zip(
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

SCENARIO("zip consecutive", "[zip][join][operators]"){
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
                        .zip(
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
                    on.next(235, 2 + 6),
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

SCENARIO("zip consecutive ends with error left", "[zip][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("zip on_error from source");

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
                        .zip(
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

SCENARIO("zip consecutive ends with error right", "[zip][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("zip on_error from source");

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
                        .zip(
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
                    on.next(235, 2 + 6),
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

SCENARIO("zip next+error/error", "[zip][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex1("zip on_error from source 1");
        std::runtime_error ex2("zip on_error from source 2");

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
                        .zip(
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

SCENARIO("zip error/next+error", "[zip][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex1("zip on_error from source 1");
        std::runtime_error ex2("zip on_error from source 2");

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
                        .zip(
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

SCENARIO("zip error after completed left", "[zip][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("zip on_error from source");

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
                        .zip(
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

SCENARIO("zip error after completed right", "[zip][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("zip on_error from source");

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
                        .zip(
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
