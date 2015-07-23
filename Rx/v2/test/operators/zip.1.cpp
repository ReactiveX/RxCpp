#include "rxcpp/rx.hpp"
namespace rxu=rxcpp::util;
namespace rxsc=rxcpp::schedulers;

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("zip never/never", "[zip][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto n1 = sc.make_hot_observable({
            on.next(150, 1)
        });

        auto n2 = sc.make_hot_observable({
            on.next(150, 1)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return n1
                        .zip(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            n2
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

            THEN("there was one subscription and one unsubscription to the n1"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 1000)
                });
                auto actual = n1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the n2"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 1000)
                });
                auto actual = n2.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("zip never N", "[zip][join][operators]"){
    GIVEN("N never completed hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        const std::size_t N = 16;

        std::vector<rxcpp::test::testable_observable<int>> n;
        for (std::size_t i = 0; i < N; ++i) {
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
                        .zip(
                            [](int v0, int v1, int v2, int v3, int v4, int v5, int v6, int v7, int v8, int v9, int v10, int v11, int v12, int v13, int v14, int v15){
                                return v0 + v1 + v2 + v3 + v4 + v5 + v6 + v7 + v8 + v9 + v10 + v11 + v12 + v13 + v14 + v15;
                            },
                            n[1], n[2], n[3], n[4], n[5], n[6], n[7], n[8], n[9], n[10], n[11], n[12], n[13], n[14], n[15]
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

SCENARIO("zip never/empty", "[zip][join][operators]"){
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
                        .zip(
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

SCENARIO("zip empty/never", "[zip][join][operators]"){
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
                        .zip(
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

SCENARIO("zip empty/empty", "[zip][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        auto e1 = sc.make_hot_observable({
            on.next(150, 1),
            on.completed(210)
        });

        auto e2 = sc.make_hot_observable({
            on.next(150, 1),
            on.completed(210)
        });

        WHEN("each int is combined with the latest from the other source"){

            auto res = w.start(
                [&]() {
                    return e1
                        .zip(
                            [](int v2, int v1){
                                return v2 + v1;
                            },
                            e2
                        )
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains only complete message"){
                auto required = rxu::to_vector({
                    on.completed(210)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the e"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 210)
                });
                auto actual = e1.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the n"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 210)
                });
                auto actual = e2.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("zip empty N", "[zip][join][operators]"){
    GIVEN("N empty hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        const int N = 16;

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
                        .zip(
                            [](int v0, int v1, int v2, int v3, int v4, int v5, int v6, int v7, int v8, int v9, int v10, int v11, int v12, int v13, int v14, int v15){
                                return v0 + v1 + v2 + v3 + v4 + v5 + v6 + v7 + v8 + v9 + v10 + v11 + v12 + v13 + v14 + v15;
                            },
                            e[1], e[2], e[3], e[4], e[5], e[6], e[7], e[8], e[9], e[10], e[11], e[12], e[13], e[14], e[15]
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

SCENARIO("zip empty/return", "[zip][join][operators]"){
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
                        .zip(
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

SCENARIO("zip return/empty", "[zip][join][operators]"){
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
                        .zip(
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

SCENARIO("zip never/return", "[zip][join][operators]"){
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
                        .zip(
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

SCENARIO("zip return/never", "[zip][join][operators]"){
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
                        .zip(
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

SCENARIO("zip return/return", "[zip][join][operators]"){
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
                        .zip(
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
                    on.next(220, 2 + 3),
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

SCENARIO("zip empty/error", "[zip][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("zip on_error from source");

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

SCENARIO("zip error/empty", "[zip][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("zip on_error from source");

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
                        .zip(
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

SCENARIO("zip never/error", "[zip][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("zip on_error from source");

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

SCENARIO("zip error/never", "[zip][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex("zip on_error from source");

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
                        .zip(
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

SCENARIO("zip error/error", "[zip][join][operators]"){
    GIVEN("2 hot observables of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        std::runtime_error ex1("zip on_error from source 1");
        std::runtime_error ex2("zip on_error from source 2");

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
