#include "rxcpp/rx.hpp"
namespace rxu=rxcpp::util;
namespace rxsc=rxcpp::schedulers;

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("combine_latest return/return", "[combine_latest][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto o1 = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(215, 2),
            on.on_completed(230)
        });

        auto o2 = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(220, 3),
            on.on_completed(240)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return o1
                        .combine_latest(
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
                    on.on_next(220, 2 + 3),
                    on.on_completed(240)
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

SCENARIO("combine_latest empty/error", "[combine_latest][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("combine_latest on_error from source");

        auto emp = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_completed(230)
        });

        auto err = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_error(220, ex)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return emp
                        .combine_latest(
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
                    on.on_error(220, ex)
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

SCENARIO("combine_latest error/empty", "[combine_latest][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("combine_latest on_error from source");

        auto err = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_error(220, ex)
        });

        auto emp = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_completed(230)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return err
                        .combine_latest(
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
                    on.on_error(220, ex)
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

SCENARIO("combine_latest return/error", "[combine_latest][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("combine_latest on_error from source");

        auto o = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(210, 2),
            on.on_completed(230)
        });

        auto err = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_error(220, ex)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return o
                        .combine_latest(
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
                    on.on_error(220, ex)
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

SCENARIO("combine_latest error/return", "[combine_latest][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("combine_latest on_error from source");

        auto err = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_error(220, ex)
        });

        auto ret = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(210, 2),
            on.on_completed(230)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return err
                        .combine_latest(
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
                    on.on_error(220, ex)
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

SCENARIO("combine_latest error/error", "[combine_latest][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex1("combine_latest on_error from source 1");
        std::runtime_error ex2("combine_latest on_error from source 2");

        auto err1 = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_error(220, ex1)
        });

        auto err2 = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_error(230, ex2)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return err1
                        .combine_latest(
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
                    on.on_error(220, ex1)
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

SCENARIO("combine_latest next+error/error", "[combine_latest][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex1("combine_latest on_error from source 1");
        std::runtime_error ex2("combine_latest on_error from source 2");

        auto err1 = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(210, 2),
            on.on_error(220, ex1)
        });

        auto err2 = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_error(230, ex2)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return err1
                        .combine_latest(
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
                    on.on_error(220, ex1)
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

SCENARIO("combine_latest error/next+error", "[combine_latest][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex1("combine_latest on_error from source 1");
        std::runtime_error ex2("combine_latest on_error from source 2");

        auto err1 = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_error(230, ex1)
        });

        auto err2 = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(210, 2),
            on.on_error(220, ex2)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return err1
                        .combine_latest(
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
                    on.on_error(220, ex2)
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

SCENARIO("combine_latest never/error", "[combine_latest][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("combine_latest on_error from source");

        auto n = sc.make_hot_observable({
            on.on_next(150, 1)
        });

        auto err = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_error(220, ex)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return n
                        .combine_latest(
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
                    on.on_error(220, ex)
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

SCENARIO("combine_latest error/never", "[combine_latest][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("combine_latest on_error from source");

        auto err = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_error(220, ex)
        });

        auto n = sc.make_hot_observable({
            on.on_next(150, 1)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return err
                        .combine_latest(
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
                    on.on_error(220, ex)
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

SCENARIO("combine_latest error after completed left", "[combine_latest][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("combine_latest on_error from source");

        auto ret = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(210, 2),
            on.on_completed(215)
        });

        auto err = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_error(220, ex)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return ret
                        .combine_latest(
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
                    on.on_error(220, ex)
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

SCENARIO("combine_latest error after completed right", "[combine_latest][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("combine_latest on_error from source");

        auto err = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_error(220, ex)
        });

        auto ret = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(210, 2),
            on.on_completed(215)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return err
                        .combine_latest(
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
                    on.on_error(220, ex)
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

SCENARIO("combine_latest selector throws", "[combine_latest][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("combine_latest on_error from source");

        auto o1 = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(215, 2),
            on.on_completed(230)
        });

        auto o2 = sc.make_hot_observable({
            on.on_next(150, 1),
            on.on_next(220, 3),
            on.on_completed(240)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return o1
                        .combine_latest(
                            [&ex](int v2, int v1) -> int {
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
                    on.on_error(220, ex)
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

SCENARIO("combine_latest selector throws N", "[combine_latest][join][operators]"){
    GIVEN("N hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        const int N = 16;

        std::runtime_error ex("combine_latest on_error from source");

        std::vector<rxcpp::test::testable_observable<int>> e;
        for (int i = 0; i < N; ++i) {
            e.push_back(
                sc.make_hot_observable({
                    on.on_next(210 + 10 * i, 1),
                    on.on_completed(500)
                })
            );
        }

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return e[0]
                        .combine_latest(
                            [&ex](int v0, int v1, int v2, int v3, int v4, int v5, int v6, int v7, int v8, int v9, int v10, int v11, int v12, int v13, int v14, int v15) -> int {
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
                    on.on_error(200 + 10 * N, ex)
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

SCENARIO("combine_latest typical N", "[combine_latest][join][operators]"){
    GIVEN("N hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        const int N = 16;

        std::vector<rxcpp::test::testable_observable<int>> o;
        for (int i = 0; i < N; ++i) {
            o.push_back(
                sc.make_hot_observable({
                    on.on_next(150, 1),
                    on.on_next(210 + 10 * i, i + 1),
                    on.on_next(410 + 10 * i, i + N + 1),
                    on.on_completed(800)
                })
            );
        }

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return o[0]
                        .combine_latest(
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
                    on.on_next(200 + 10 * N, N * (N + 1) / 2)
                });
                for (int i = 0; i < N; ++i) {
                    required.push_back(on.on_next(410 + 10 * i, N * (N + 1) / 2 + N + N * i));
                }
                required.push_back(on.on_completed(800));
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
