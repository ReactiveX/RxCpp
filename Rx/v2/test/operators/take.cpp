#include "rxcpp/rx.hpp"
namespace rx=rxcpp;
namespace rxu=rxcpp::util;
namespace rxo=rxcpp::operators;
namespace rxs=rxcpp::sources;
namespace rxsc=rxcpp::schedulers;
namespace rxsub=rxcpp::subjects;
namespace rxn=rxcpp::notifications;

#include "rxcpp/rx-test.hpp"
namespace rxt=rxcpp::test;

#include "catch.hpp"

SCENARIO("take 2", "[take][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        record xmessages[] = {
            on_next(150, 1),
            on_next(210, 2),
            on_next(220, 3),
            on_next(230, 4),
            on_next(240, 5),
            on_completed(250)
        };
        auto xs = sc.make_hot_observable(xmessages);

        WHEN("2 values are taken"){

            auto res = w.start<int>(
                [xs]() {
                    return xs
                        .take(2)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_next(210, 2),
                    on_next(220, 3),
                    on_completed(220)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                life items[] = {
                    subscribe(200, 220)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take, complete after", "[take][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        record xmessages[] = {
            on_next(70, 6),
            on_next(150, 4),
            on_next(210, 9),
            on_next(230, 13),
            on_next(270, 7),
            on_next(280, 1),
            on_next(300, -1),
            on_next(310, 3),
            on_next(340, 8),
            on_next(370, 11),
            on_next(410, 15),
            on_next(415, 16),
            on_next(460, 72),
            on_next(510, 76),
            on_next(560, 32),
            on_next(570, -100),
            on_next(580, -3),
            on_next(590, 5),
            on_next(630, 10),
            on_completed(690)
        };
        auto xs = sc.make_hot_observable(xmessages);

        WHEN("20 values are taken"){

            auto res = w.start<int>(
                [xs]() {
                    return xs
                        .take(20)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_next(210, 9),
                    on_next(230, 13),
                    on_next(270, 7),
                    on_next(280, 1),
                    on_next(300, -1),
                    on_next(310, 3),
                    on_next(340, 8),
                    on_next(370, 11),
                    on_next(410, 15),
                    on_next(415, 16),
                    on_next(460, 72),
                    on_next(510, 76),
                    on_next(560, 32),
                    on_next(570, -100),
                    on_next(580, -3),
                    on_next(590, 5),
                    on_next(630, 10),
                    on_completed(690)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                life items[] = {
                    subscribe(200, 690)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take, complete same", "[take][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        record xmessages[] = {
            on_next(70, 6),
            on_next(150, 4),
            on_next(210, 9),
            on_next(230, 13),
            on_next(270, 7),
            on_next(280, 1),
            on_next(300, -1),
            on_next(310, 3),
            on_next(340, 8),
            on_next(370, 11),
            on_next(410, 15),
            on_next(415, 16),
            on_next(460, 72),
            on_next(510, 76),
            on_next(560, 32),
            on_next(570, -100),
            on_next(580, -3),
            on_next(590, 5),
            on_next(630, 10),
            on_completed(690)
        };
        auto xs = sc.make_hot_observable(xmessages);

        WHEN("17 values are taken"){

            auto res = w.start<int>(
                [xs]() {
                    return xs
                        .take(17)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_next(210, 9),
                    on_next(230, 13),
                    on_next(270, 7),
                    on_next(280, 1),
                    on_next(300, -1),
                    on_next(310, 3),
                    on_next(340, 8),
                    on_next(370, 11),
                    on_next(410, 15),
                    on_next(415, 16),
                    on_next(460, 72),
                    on_next(510, 76),
                    on_next(560, 32),
                    on_next(570, -100),
                    on_next(580, -3),
                    on_next(590, 5),
                    on_next(630, 10),
                    on_completed(630)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                life items[] = {
                    subscribe(200, 630)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take, complete before", "[take][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        record xmessages[] = {
            on_next(70, 6),
            on_next(150, 4),
            on_next(210, 9),
            on_next(230, 13),
            on_next(270, 7),
            on_next(280, 1),
            on_next(300, -1),
            on_next(310, 3),
            on_next(340, 8),
            on_next(370, 11),
            on_next(410, 15),
            on_next(415, 16),
            on_next(460, 72),
            on_next(510, 76),
            on_next(560, 32),
            on_next(570, -100),
            on_next(580, -3),
            on_next(590, 5),
            on_next(630, 10),
            on_completed(690)
        };
        auto xs = sc.make_hot_observable(xmessages);

        WHEN("10 values are taken"){

            auto res = w.start<int>(
                [xs]() {
                    return xs
                        .take(10)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_next(210, 9),
                    on_next(230, 13),
                    on_next(270, 7),
                    on_next(280, 1),
                    on_next(300, -1),
                    on_next(310, 3),
                    on_next(340, 8),
                    on_next(370, 11),
                    on_next(410, 15),
                    on_next(415, 16),
                    on_completed(415)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                life items[] = {
                    subscribe(200, 415)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take, error after", "[take][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        std::runtime_error ex("take on_error from source");

        record xmessages[] = {
            on_next(70, 6),
            on_next(150, 4),
            on_next(210, 9),
            on_next(230, 13),
            on_next(270, 7),
            on_next(280, 1),
            on_next(300, -1),
            on_next(310, 3),
            on_next(340, 8),
            on_next(370, 11),
            on_next(410, 15),
            on_next(415, 16),
            on_next(460, 72),
            on_next(510, 76),
            on_next(560, 32),
            on_next(570, -100),
            on_next(580, -3),
            on_next(590, 5),
            on_next(630, 10),
            on_error(690, ex)
        };
        auto xs = sc.make_hot_observable(xmessages);

        WHEN("20 values are taken"){

            auto res = w.start<int>(
                [xs]() {
                    return xs
                        .take(20)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_next(210, 9),
                    on_next(230, 13),
                    on_next(270, 7),
                    on_next(280, 1),
                    on_next(300, -1),
                    on_next(310, 3),
                    on_next(340, 8),
                    on_next(370, 11),
                    on_next(410, 15),
                    on_next(415, 16),
                    on_next(460, 72),
                    on_next(510, 76),
                    on_next(560, 32),
                    on_next(570, -100),
                    on_next(580, -3),
                    on_next(590, 5),
                    on_next(630, 10),
                    on_error(690, ex)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                life items[] = {
                    subscribe(200, 690)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take, error same", "[take][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        record xmessages[] = {
            on_next(70, 6),
            on_next(150, 4),
            on_next(210, 9),
            on_next(230, 13),
            on_next(270, 7),
            on_next(280, 1),
            on_next(300, -1),
            on_next(310, 3),
            on_next(340, 8),
            on_next(370, 11),
            on_next(410, 15),
            on_next(415, 16),
            on_next(460, 72),
            on_next(510, 76),
            on_next(560, 32),
            on_next(570, -100),
            on_next(580, -3),
            on_next(590, 5),
            on_next(630, 10),
            on_error(690, std::runtime_error("error in unsubscribed stream"))
        };
        auto xs = sc.make_hot_observable(xmessages);

        WHEN("17 values are taken"){

            auto res = w.start<int>(
                [xs]() {
                    return xs
                        .take(17)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_next(210, 9),
                    on_next(230, 13),
                    on_next(270, 7),
                    on_next(280, 1),
                    on_next(300, -1),
                    on_next(310, 3),
                    on_next(340, 8),
                    on_next(370, 11),
                    on_next(410, 15),
                    on_next(415, 16),
                    on_next(460, 72),
                    on_next(510, 76),
                    on_next(560, 32),
                    on_next(570, -100),
                    on_next(580, -3),
                    on_next(590, 5),
                    on_next(630, 10),
                    on_completed(630)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                life items[] = {
                    subscribe(200, 630)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take, error before", "[take][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        record xmessages[] = {
            on_next(70, 6),
            on_next(150, 4),
            on_next(210, 9),
            on_next(230, 13),
            on_next(270, 7),
            on_next(280, 1),
            on_next(300, -1),
            on_next(310, 3),
            on_next(340, 8),
            on_next(370, 11),
            on_next(410, 15),
            on_next(415, 16),
            on_next(460, 72),
            on_next(510, 76),
            on_next(560, 32),
            on_next(570, -100),
            on_next(580, -3),
            on_next(590, 5),
            on_next(630, 10),
            on_error(690, std::runtime_error("error in unsubscribed stream"))
        };
        auto xs = sc.make_hot_observable(xmessages);

        WHEN("3 values are taken"){

            auto res = w.start<int>(
                [xs]() {
                    return xs
                        .take(3)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_next(210, 9),
                    on_next(230, 13),
                    on_next(270, 7),
                    on_completed(270)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                life items[] = {
                    subscribe(200, 270)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take, dispose before", "[take][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        record xmessages[] = {
            on_next(70, 6),
            on_next(150, 4),
            on_next(210, 9),
            on_next(230, 13),
            on_next(270, 7),
            on_next(280, 1),
            on_next(300, -1),
            on_next(310, 3),
            on_next(340, 8),
            on_next(370, 11),
            on_next(410, 15),
            on_next(415, 16),
            on_next(460, 72),
            on_next(510, 76),
            on_next(560, 32),
            on_next(570, -100),
            on_next(580, -3),
            on_next(590, 5),
            on_next(630, 10)
        };
        auto xs = sc.make_hot_observable(xmessages);

        WHEN("3 values are taken"){

            auto res = w.start<int>(
                [xs]() {
                    return xs
                        .take(3)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                250
            );

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_next(210, 9),
                    on_next(230, 13)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                life items[] = {
                    subscribe(200, 250)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

SCENARIO("take, dispose after", "[take][operators]"){
    GIVEN("a source"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        record xmessages[] = {
            on_next(70, 6),
            on_next(150, 4),
            on_next(210, 9),
            on_next(230, 13),
            on_next(270, 7),
            on_next(280, 1),
            on_next(300, -1),
            on_next(310, 3),
            on_next(340, 8),
            on_next(370, 11),
            on_next(410, 15),
            on_next(415, 16),
            on_next(460, 72),
            on_next(510, 76),
            on_next(560, 32),
            on_next(570, -100),
            on_next(580, -3),
            on_next(590, 5),
            on_next(630, 10)
        };
        auto xs = sc.make_hot_observable(xmessages);

        WHEN("3 values are taken"){

            auto res = w.start<int>(
                [xs]() {
                    return xs
                        .take(3)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                },
                400
            );

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_next(210, 9),
                    on_next(230, 13),
                    on_next(270, 7),
                    on_completed(270)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                life items[] = {
                    subscribe(200, 270)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}



SCENARIO("take_until trigger on_next", "[take_until][take][operators]"){
    GIVEN("2 sources"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        record xmessages[] = {
            on_next(150, 1),
            on_next(210, 2),
            on_next(220, 3),
            on_next(230, 4),
            on_next(240, 5),
            on_completed(250)
        };
        auto xs = sc.make_hot_observable(xmessages);

        record ymessages[] = {
            on_next(150, 1),
            on_next(225, 99),
            on_completed(230)
        };
        auto ys = sc.make_hot_observable(ymessages);

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start<int>(
                [xs, ys]() {
                    return xs
                        .take_until(ys)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_next(210, 2),
                    on_next(220, 3),
                    on_completed(225)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                life items[] = {
                    subscribe(200, 225)
                };
                auto required = rxu::to_vector(items);
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                life items[] = {
                    subscribe(200, 225)
                };
                auto required = rxu::to_vector(items);
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
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        record lmessages[] = {
            on_next(150, 1),
            on_next(210, 2),
            on_next(220, 3),
            on_next(230, 4),
            on_next(240, 5),
            on_completed(250)
        };
        auto l = sc.make_hot_observable(lmessages);

        record rmessages[] = {
            on_next(150, 1),
            on_next(225, 99),
            on_completed(230)
        };
        auto r = sc.make_hot_observable(rmessages);

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start<int>(
                [l, r]() {
                    return l
                        .take_until(r)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_next(210, 2),
                    on_next(220, 3),
                    on_completed(225)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                life items[] = {
                    subscribe(200, 225)
                };
                auto required = rxu::to_vector(items);
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                life items[] = {
                    subscribe(200, 225)
                };
                auto required = rxu::to_vector(items);
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
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        std::runtime_error ex("take_until on_error from source");

        record lmessages[] = {
            on_next(150, 1),
            on_next(210, 2),
            on_next(220, 3),
            on_next(230, 4),
            on_next(240, 5),
            on_completed(250)
        };
        auto l = sc.make_hot_observable(lmessages);

        record rmessages[] = {
            on_next(150, 1),
            on_error(225, ex)
        };
        auto r = sc.make_hot_observable(rmessages);

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start<int>(
                [l, r]() {
                    return l
                        .take_until(r)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_next(210, 2),
                    on_next(220, 3),
                    on_error(225, ex)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                life items[] = {
                    subscribe(200, 225)
                };
                auto required = rxu::to_vector(items);
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                life items[] = {
                    subscribe(200, 225)
                };
                auto required = rxu::to_vector(items);
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
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        record lmessages[] = {
            on_next(150, 1),
            on_next(210, 2),
            on_next(220, 3),
            on_next(230, 4),
            on_next(240, 5),
            on_completed(250)
        };
        auto l = sc.make_hot_observable(lmessages);

        record rmessages[] = {
            on_next(150, 1),
            on_completed(225)
        };
        auto r = sc.make_hot_observable(rmessages);

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start<int>(
                [l, r]() {
                    return l
                        .take_until(r)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_next(210, 2),
                    on_next(220, 3),
                    on_next(230, 4),
                    on_next(240, 5),
                    on_completed(250)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                life items[] = {
                    subscribe(200, 250)
                };
                auto required = rxu::to_vector(items);
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                life items[] = {
                    subscribe(200, 225)
                };
                auto required = rxu::to_vector(items);
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
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        record lmessages[] = {
            on_next(150, 1),
            on_next(210, 2),
            on_next(220, 3),
            on_next(230, 4),
            on_next(240, 5),
            on_completed(250)
        };
        auto l = sc.make_hot_observable(lmessages);

        record rmessages[] = {
            on_next(150, 1)
        };
        auto r = sc.make_hot_observable(rmessages);

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start<int>(
                [l, r]() {
                    return l
                        .take_until(r)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_next(210, 2),
                    on_next(220, 3),
                    on_next(230, 4),
                    on_next(240, 5),
                    on_completed(250)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                life items[] = {
                    subscribe(200, 250)
                };
                auto required = rxu::to_vector(items);
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                life items[] = {
                    subscribe(200, 250)
                };
                auto required = rxu::to_vector(items);
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
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        record lmessages[] = {
            on_next(150, 1)
        };
        auto l = sc.make_hot_observable(lmessages);

        record rmessages[] = {
            on_next(150, 1),
            on_next(225, 2), //!
            on_completed(250)
        };
        auto r = sc.make_hot_observable(rmessages);

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start<int>(
                [l, r]() {
                    return l
                        .take_until(r)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_completed(225)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                life items[] = {
                    subscribe(200, 225)
                };
                auto required = rxu::to_vector(items);
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                life items[] = {
                    subscribe(200, 225)
                };
                auto required = rxu::to_vector(items);
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
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        std::runtime_error ex("take_until on_error from source");

        record lmessages[] = {
            on_next(150, 1)
        };
        auto l = sc.make_hot_observable(lmessages);

        record rmessages[] = {
            on_next(150, 1),
            on_error(225, ex)
        };
        auto r = sc.make_hot_observable(rmessages);

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start<int>(
                [l, r]() {
                    return l
                        .take_until(r)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_error(225, ex)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                life items[] = {
                    subscribe(200, 225)
                };
                auto required = rxu::to_vector(items);
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                life items[] = {
                    subscribe(200, 225)
                };
                auto required = rxu::to_vector(items);
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
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        record lmessages[] = {
            on_next(150, 1)
        };
        auto l = sc.make_hot_observable(lmessages);

        record rmessages[] = {
            on_next(150, 1),
            on_completed(225)
        };
        auto r = sc.make_hot_observable(rmessages);

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start<int>(
                [l, r]() {
                    return l
                        .take_until(r)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = std::vector<record>();
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                life items[] = {
                    subscribe(200, 1000 /* can't dispose prematurely, could be in flight to dispatch OnError */)
                };
                auto required = rxu::to_vector(items);
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                life items[] = {
                    subscribe(200, 225)
                };
                auto required = rxu::to_vector(items);
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
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        record lmessages[] = {
            on_next(150, 1)
        };
        auto l = sc.make_hot_observable(lmessages);

        record rmessages[] = {
            on_next(150, 1)
        };
        auto r = sc.make_hot_observable(rmessages);

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start<int>(
                [l, r]() {
                    return l
                        .take_until(r)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                auto required = std::vector<record>();
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                life items[] = {
                    subscribe(200, 1000)
                };
                auto required = rxu::to_vector(items);
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                life items[] = {
                    subscribe(200, 1000)
                };
                auto required = rxu::to_vector(items);
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
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        record lmessages[] = {
            on_next(150, 1),
            on_next(230, 2),
            on_completed(240)
        };
        auto l = sc.make_hot_observable(lmessages);

        record rmessages[] = {
            on_next(150, 1),
            on_next(210, 2), //!
            on_completed(220)
        };
        auto r = sc.make_hot_observable(rmessages);

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start<int>(
                [l, r]() {
                    return l
                        .take_until(r)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_completed(210)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                life items[] = {
                    subscribe(200, 210)
                };
                auto required = rxu::to_vector(items);
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                life items[] = {
                    subscribe(200, 210)
                };
                auto required = rxu::to_vector(items);
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
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        bool sourceNotDisposed = false;

        record lmessages[] = {
            on_next(150, 1),
            on_error(215, std::runtime_error("error in unsubscribed stream")), // should not come
            on_completed(240)
        };
        auto l = sc.make_hot_observable(lmessages);

        record rmessages[] = {
            on_next(150, 1),
            on_next(210, 2), //!
            on_completed(220)
        };
        auto r = sc.make_hot_observable(rmessages);

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start<int>(
                [l, r, &sourceNotDisposed]() {
                    return l
                        .map([&sourceNotDisposed](int v){sourceNotDisposed = true; return v;})
                        .take_until(r)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_completed(210)
                };
                auto required = rxu::to_vector(items);
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
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        bool signalNotDisposed = false;

        record lmessages[] = {
            on_next(150, 1),
            on_next(230, 2),
            on_completed(240)
        };
        auto l = sc.make_hot_observable(lmessages);

        record rmessages[] = {
            on_next(150, 1),
            on_next(250, 2),
            on_completed(260)
        };
        auto r = sc.make_hot_observable(rmessages);

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start<int>(
                [l, r, &signalNotDisposed]() {
                    return l
                        .take_until(r
                            .map([&signalNotDisposed](int v){signalNotDisposed = true; return v;}))
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_next(230, 2),
                    on_completed(240)
                };
                auto required = rxu::to_vector(items);
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
        typedef rxsc::test::messages<int> m;
        typedef rxn::subscription life;
        typedef m::recorded_type record;
        auto on_next = m::on_next;
        auto on_error = m::on_error;
        auto on_completed = m::on_completed;
        auto subscribe = m::subscribe;

        std::runtime_error ex("take_until on_error from source");

        record lmessages[] = {
            on_next(150, 1),
            on_error(225, ex)
        };
        auto l = sc.make_hot_observable(lmessages);

        record rmessages[] = {
            on_next(150, 1),
            on_next(240, 2)
        };
        auto r = sc.make_hot_observable(rmessages);

        WHEN("one is taken until the other emits a marble"){

            auto res = w.start<int>(
                [l, r]() {
                    return l
                        .take_until(r)
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output only contains items sent while subscribed"){
                record items[] = {
                    on_error(225, ex)
                };
                auto required = rxu::to_vector(items);
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the source"){
                life items[] = {
                    subscribe(200, 225)
                };
                auto required = rxu::to_vector(items);
                auto actual = l.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("there was 1 subscription/unsubscription to the trigger"){
                life items[] = {
                    subscribe(200, 225)
                };
                auto required = rxu::to_vector(items);
                auto actual = r.subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}
