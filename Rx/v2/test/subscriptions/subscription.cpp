#include "rxcpp/rx.hpp"
namespace rx=rxcpp;

#include "catch.hpp"

SCENARIO("subscription traits", "[subscription][traits]"){
    GIVEN("given some subscription types"){
        auto empty = [](){};
        auto es = rx::make_subscription();
        rx::composite_subscription cs;
        WHEN("tested"){
            THEN("is_subscription value is true for empty subscription"){
                REQUIRE(rx::is_subscription<decltype(es)>::value);
            }
            THEN("is_subscription value is true for composite_subscription"){
                REQUIRE(rx::is_subscription<decltype(cs)>::value);
            }
        }
    }
}

SCENARIO("non-subscription traits", "[subscription][traits]"){
    GIVEN("given some non-subscription types"){
        auto l = [](){};
        int i = 0;
        void* v = nullptr;
        WHEN("tested"){
            THEN("is_subscription value is false for lambda"){
                REQUIRE(!rx::is_subscription<decltype(l)>::value);
            }
            THEN("is_subscription value is false for int"){
                REQUIRE(!rx::is_subscription<decltype(i)>::value);
            }
            THEN("is_subscription value is false for void*"){
                REQUIRE(!rx::is_subscription<decltype(v)>::value);
            }
            THEN("is_subscription value is false for void"){
                REQUIRE(!rx::is_subscription<void>::value);
            }
        }
    }
}

SCENARIO("subscription static", "[subscription]"){
    GIVEN("given a subscription"){
        int i=0;
        auto s = rx::make_subscription([&i](){++i;});
        WHEN("not used"){
            THEN("is subscribed"){
                REQUIRE(s.is_subscribed());
            }
            THEN("i is 0"){
                REQUIRE(i == 0);
            }
        }
        WHEN("used"){
            THEN("is not subscribed when unsubscribed once"){
                s.unsubscribe();
                REQUIRE(!s.is_subscribed());
            }
            THEN("is not subscribed when unsubscribed twice"){
                s.unsubscribe();
                s.unsubscribe();
                REQUIRE(!s.is_subscribed());
            }
            THEN("i is 1 when unsubscribed once"){
                s.unsubscribe();
                REQUIRE(i == 1);
            }
            THEN("i is 1 when unsubscribed twice"){
                s.unsubscribe();
                s.unsubscribe();
                REQUIRE(i == 1);
            }
        }
    }
}

SCENARIO("subscription empty", "[subscription]"){
    GIVEN("given an empty subscription"){
        auto s = rx::make_subscription();
        WHEN("not used"){
            THEN("is not subscribed"){
                REQUIRE(!s.is_subscribed());
            }
        }
        WHEN("used"){
            THEN("is not subscribed when unsubscribed once"){
                s.unsubscribe();
                REQUIRE(!s.is_subscribed());
            }
            THEN("is not subscribed when unsubscribed twice"){
                s.unsubscribe();
                s.unsubscribe();
                REQUIRE(!s.is_subscribed());
            }
        }
    }
}

SCENARIO("subscription composite", "[subscription]"){
    GIVEN("given a subscription"){
        int i=0;
        rx::composite_subscription s;
        s.add(rx::make_subscription());
        s.add(rx::make_subscription([&i](){++i;}));
        s.add([&i](){++i;});
        WHEN("not used"){
            THEN("is subscribed"){
                REQUIRE(s.is_subscribed());
            }
            THEN("i is 0"){
                REQUIRE(i == 0);
            }
        }
        WHEN("used"){
            THEN("is not subscribed when unsubscribed once"){
                s.unsubscribe();
                REQUIRE(!s.is_subscribed());
            }
            THEN("is not subscribed when unsubscribed twice"){
                s.unsubscribe();
                s.unsubscribe();
                REQUIRE(!s.is_subscribed());
            }
            THEN("i is 2 when unsubscribed once"){
                s.unsubscribe();
                REQUIRE(i == 2);
            }
            THEN("i is 2 when unsubscribed twice"){
                s.unsubscribe();
                s.unsubscribe();
                REQUIRE(i == 2);
            }
        }
    }
}

