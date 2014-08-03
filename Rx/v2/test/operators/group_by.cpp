#include "rxcpp/rx.hpp"
namespace rxu=rxcpp::util;
namespace rxs=rxcpp::sources;
namespace rxsc=rxcpp::schedulers;

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

#include <locale>

char whitespace(char c) {
    return std::isspace<char>(c, std::locale::classic());
}

std::string trim(std::string s) {
    auto first = std::find_if_not(s.begin(), s.end(), whitespace);
    auto last = std::find_if_not(s.rbegin(), s.rend(), whitespace);
    if (last != s.rend()) {
        s.erase(s.end() - (last-s.rbegin()), s.end());
    }
    s.erase(s.begin(), first);
    return std::move(s);
}

bool tolowerLess(char lhs, char rhs) {
    return std::tolower(lhs, std::locale::classic()) < std::tolower(rhs, std::locale::classic());
}

bool tolowerStringLess(const std::string& lhs, const std::string& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), tolowerLess);
}

SCENARIO("group_by", "[group_by][operators]"){
    GIVEN("1 hot observable of ints."){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<std::string> on;
        int keyInvoked = 0;
        int marbleInvoked = 0;

        auto xs = sc.make_hot_observable({
            on.on_next(90, "error"),
            on.on_next(110, "error"),
            on.on_next(130, "error"),
            on.on_next(220, "  foo"),
            on.on_next(240, " FoO "),
            on.on_next(270, "baR  "),
            on.on_next(310, "foO "),
            on.on_next(350, " Baz   "),
            on.on_next(360, "  qux "),
            on.on_next(390, "   bar"),
            on.on_next(420, " BAR  "),
            on.on_next(470, "FOO "),
            on.on_next(480, "baz  "),
            on.on_next(510, " bAZ "),
            on.on_next(530, "    fOo    "),
            on.on_completed(570),
            on.on_next(580, "error"),
            on.on_completed(600),
            on.on_error(650, new std::runtime_error("error in completed sequence"))
        });

        WHEN("group each int with the next 2 ints"){

            auto res = w.start(
                [&]() {
                    return xs
                        .group_by(
                            [&](std::string v){
                                ++keyInvoked;
                                return trim(std::move(v));
                            },
                            [&](std::string v){
                                ++marbleInvoked;
                                std::reverse(v.begin(), v.end());
                                return v;
                            },
                            tolowerStringLess)
                        .map([](const rxcpp::grouped_observable<std::string, std::string>& g){return g.get_key();})
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains groups of ints"){
                auto required = rxu::to_vector({
                    on.on_next(220, "foo"),
                    on.on_next(270, "baR"),
                    on.on_next(350, "Baz"),
                    on.on_next(360, "qux"),
                    on.on_completed(570)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription to the xs"){
                auto required = rxu::to_vector({
                    on.subscribe(200, 570)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }

            THEN("key selector was invoked for each value"){
                REQUIRE(12 == keyInvoked);
            }

            THEN("marble selector was invoked for each value"){
                REQUIRE(12 == marbleInvoked);
            }
        }
    }
}
