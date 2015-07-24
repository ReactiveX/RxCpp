#include "../test.h"

SCENARIO("subscriber traits", "[observer][traits]"){
    GIVEN("given some subscriber types"){
        int result = 0;
        auto next = [&result](int i){result += i;};
        auto error = [&result](std::exception_ptr){result += 10;};
        auto completed = [&result](){result += 100;};
//        auto ra = rx::rxu::detail::arg_resolver_n<0, rx::tag_resumption_resolution::template predicate, typename rx::tag_resumption_resolution::default_type, rx::resumption, decltype(next), decltype(error), decltype(completed), rx::rxu::detail::tag_unresolvable, rx::rxu::detail::tag_unresolvable>(rx::resumption(), next, error, completed, rx::rxu::detail::tag_unresolvable(), rx::rxu::detail::tag_unresolvable());
//        auto ra = typename rx::rxu::detail::arg_resolver<rx::tag_resumption_resolution::template predicate, typename rx::tag_resumption_resolution::default_type, rx::resumption, decltype(next), decltype(error), decltype(completed)>::type(rx::resumption(), next, error, completed, rx::rxu::detail::tag_unresolvable(), rx::rxu::detail::tag_unresolvable());
//        auto arg = rx::rxu::detail::resolve_arg<rx::tag_resumption_resolution::template predicate, typename rx::tag_resumption_resolution::default_type>(rx::resumption(), next, error, completed);
//        auto argset = rx::rxu::detail::resolve_arg_set(rxcpp::detail::tag_subscriber_set<int>(), rx::resumption(), next, error, completed);
//        auto o = rx::detail::make_observer_resolved<int>(argset);
//        auto o = rx::detail::select_observer<int>(argset);
//        auto scrbResult = rx::subscriber<int, decltype(o)>(std::move(std::get<0>(argset).value), std::move(std::get<1>(argset).value), o);
//        static_assert(std::tuple_element<1, decltype(argset)>::type::is_arg, "resumption is a required parameter");
//        auto scrbResult = rx::detail::make_subscriber_resolved<int>(rx::rxu::detail::resolve_arg_set(rx::detail::tag_subscriber_set<int>(), rx::resumption(), next, error, completed));
//        auto scrbResult = rx::detail::make_subscriber_resolved<int>(argset);
        auto scrbResult = rx::make_subscriber<int>(next, error, completed);
        auto scrbdup = rx::make_subscriber<int>(scrbResult);
        auto scrbop = rx::make_subscriber<int>(scrbResult, next, error, completed);
        auto scrbsharelifetime = rx::make_subscriber<int>(scrbResult, scrbop.get_observer());
        auto scrbuniquelifetime = rx::make_subscriber<int>(scrbResult, rx::composite_subscription());

        auto emptyNext = [](int){};
        auto scrb = rx::make_subscriber<int>(emptyNext);
        WHEN("tested"){
            THEN("is_observer value is true for subscriber"){
                REQUIRE(rx::is_observer<decltype(scrb)>::value);
            }
            THEN("is_subscription value is true for subscriber"){
                REQUIRE(rx::is_subscription<decltype(scrb)>::value);
            }
        }
        WHEN("nothing is called"){
            THEN("static_observer result is 0"){
                REQUIRE(result == 0);
            }
        }
        WHEN("onnext is called with 1"){
            THEN("subscriber result is 1"){
                scrbResult.on_next(1);
                REQUIRE(result == 1);
            }
        }
        WHEN("after error"){
            THEN("subscriber result is 10"){
                scrbResult.on_error(std::current_exception());
                REQUIRE(result == 10);
            }
        }
        WHEN("after completed"){
            THEN("subscriber result is 100"){
                scrbResult.on_completed();
                REQUIRE(result == 100);
            }
        }
    }
}

SCENARIO("observer traits", "[observer][traits]"){
    GIVEN("given some observer types"){
        auto emptyNext = [](int){};
        auto dob = rx::make_observer_dynamic<int>(emptyNext);
        auto so = rx::make_observer<int>(emptyNext);
        auto eo = rx::make_observer<int>();
        WHEN("tested"){
            THEN("is_observer value is true for dynamic_observer"){
                REQUIRE(rx::is_observer<decltype(dob)>::value);
            }
            THEN("is_observer value is true for static_observer"){
                REQUIRE(rx::is_observer<decltype(so)>::value);
            }
            THEN("is_observer value is true for observer<void>"){
                REQUIRE(rx::is_observer<decltype(eo)>::value);
            }
        }
    }
}

SCENARIO("non-observer traits", "[observer][traits]"){
    GIVEN("given some subscription types"){
        auto empty = [](){};
        rx::static_subscription<decltype(empty)> ss(empty);
        auto es = rx::make_subscription();
        rx::composite_subscription cs;
        WHEN("tested"){
            THEN("is_observer value is false for static_subscription"){
                REQUIRE(!rx::is_observer<decltype(ss)>::value);
            }
            THEN("is_observer value is false for subscription<void>"){
                REQUIRE(!rx::is_observer<decltype(es)>::value);
            }
            THEN("is_observer value is false for composite_subscription"){
                REQUIRE(!rx::is_observer<decltype(cs)>::value);
            }
        }
    }
}

SCENARIO("subscriber behavior", "[observer][traits]"){
    GIVEN("given some subscriber types"){
        int result = 0;
        auto next = [&result](int i){result += i;};
        auto error = [&result](std::exception_ptr){result += 10;};
        auto completed = [&result](){result += 100;};
        auto dob = rx::make_subscriber<int>(rx::make_observer_dynamic<int>(next, error, completed));
        auto so = rx::make_subscriber<int>(next, error, completed);
        WHEN("nothing is called"){
            THEN("dynamic_observer result is 0"){
                REQUIRE(result == 0);
            }
            THEN("static_observer result is 0"){
                REQUIRE(result == 0);
            }
            THEN("observer<void> result is 0"){
                REQUIRE(result == 0);
            }
            THEN("dynamic_observer is subscribed"){
                REQUIRE(dob.is_subscribed());
            }
            THEN("static_observer is subscribed"){
                REQUIRE(so.is_subscribed());
            }
        }
        WHEN("onnext is called with 1"){
            THEN("dynamic_observer result is 1"){
                dob.on_next(1);
                REQUIRE(result == 1);
            }
            THEN("static_observer result is 1"){
                so.on_next(1);
                REQUIRE(result == 1);
            }
            THEN("dynamic_observer is subscribed"){
                dob.on_next(1);
                REQUIRE(dob.is_subscribed());
            }
            THEN("static_observer is subscribed"){
                so.on_next(1);
                REQUIRE(so.is_subscribed());
            }
        }
        WHEN("after error"){
            THEN("dynamic_observer result is 10"){
                dob.on_error(std::current_exception());
                REQUIRE(result == 10);
            }
            THEN("static_observer result is 10"){
                so.on_error(std::current_exception());
                REQUIRE(result == 10);
            }
            THEN("dynamic_observer is not subscribed"){
                dob.on_error(std::current_exception());
                REQUIRE(!dob.is_subscribed());
            }
            THEN("static_observer is not subscribed"){
                so.on_error(std::current_exception());
                REQUIRE(!so.is_subscribed());
            }
        }
        WHEN("after completed"){
            THEN("dynamic_observer result is 100"){
                dob.on_completed();
                REQUIRE(result == 100);
            }
            THEN("static_observer result is 100"){
                so.on_completed();
                REQUIRE(result == 100);
            }
            THEN("dynamic_observer is not subscribed"){
                dob.on_completed();
                REQUIRE(!dob.is_subscribed());
            }
            THEN("static_observer is not subscribed"){
                so.on_completed();
                REQUIRE(!so.is_subscribed());
            }
        }
    }
}
