#include "rxcpp/rx.hpp"
namespace rx=rxcpp;
namespace rxo=rxcpp::operators;

#include "catch.hpp"

namespace {
bool IsPrime(int x)
{
    if (x < 2) return false;
    for (int i = 2; i <= x/2; ++i)
    {
        if (x % i == 0)
            return false;
    }
    return true;
}
}

SCENARIO("filter members", "[filter][operators][traits]"){
    GIVEN("given a range of ints from 1 to 100"){
        WHEN("filtered to primes"){
            THEN("a subscription only observes primes"){
                auto s = rx::observable<>::range(1, 100, 1)
                    .filter(IsPrime)
                    .subscribe([](int t) {
                        const auto prime = IsPrime(t) ? t : -1;
                        REQUIRE( t == prime );
                    });
            }
        }
    }
}

SCENARIO("filter operators", "[filter][operators][traits]"){
    GIVEN("given a range of ints from 1 to 100"){
        WHEN("filtered to primes"){
            THEN("a subscription only observes primes"){
                auto s = rx::observable<>::range(1, 100, 1)
                    >> rxo::filter(IsPrime)
                    >> rxo::subscribe([](int t) {
                        const auto prime = IsPrime(t) ? t : -1;
                        REQUIRE( t == prime );
                    });
            }
        }
    }
}
