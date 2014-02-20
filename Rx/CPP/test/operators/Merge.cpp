#include "cpprx/rx.hpp"
namespace rx=rxcpp;

#include "catch.hpp"

SCENARIO("merge issue 5", "[merge][issue][operators]"){
    GIVEN("Empty and Never"){
        auto scheduler = std::make_shared<rx::TestScheduler>();
        typedef rx::TestScheduler::Messages<int> m;

        WHEN("merged 4 times"){

            std::vector<int> empty;
            auto o = rx::observable(rx::from(rx::Iterate(empty))
                .merge(rx::Never<int>()));

            auto res1 = scheduler->Start<int>(
                [o]() {
                    return o;
                }
            );
            auto res2 = scheduler->Start<int>(
                [o]() {
                    return o;
                }
            );
            auto res3 = scheduler->Start<int>(
                [o]() {
                    return o;
                }
            );
            auto res4 = scheduler->Start<int>(
                [o]() {
                    return o;
                }
            );

            THEN("1 - the output is empty and subscribed"){
                std::vector<m::RecordedT> required;
                auto actual = res1->Messages();
                REQUIRE(required == actual);
            }
            THEN("2 - the output is empty and subscribed"){
                std::vector<m::RecordedT> required;
                auto actual = res2->Messages();
                REQUIRE(required == actual);
            }
            THEN("3 - the output is empty and subscribed"){
                std::vector<m::RecordedT> required;
                auto actual = res3->Messages();
                REQUIRE(required == actual);
            }
            THEN("4 - the output is empty and subscribed"){
                std::vector<m::RecordedT> required;
                auto actual = res4->Messages();
                REQUIRE(required == actual);
            }
        }
    }
}
