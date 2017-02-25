#include "../test.h"
#include "rxcpp/operators/rx-retry.hpp"

SCENARIO("retry, basic test", "[retry][operators]") {
    GIVEN("hot observable of 3x4x7 ints with errors inbetween the groups. Infinite retry.") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        std::runtime_error ex("retry on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(300, 1),
            on.next(325, 2),
            on.next(350, 3),
            on.error(400, ex),
            on.next(425, 1),
            on.next(450, 2),
            on.next(475, 3),
            on.next(500, 4),
            on.error(525, ex),
            on.next(550, 1),
            on.next(575, 2),
            on.next(600, 3),
            on.next(625, 4),
            on.next(650, 5),
            on.next(675, 6),
            on.next(700, 7),
            on.completed(725)
        });

        WHEN("infinite retry is launched") {

            auto res = w.start(
                [&]() {
                return xs
                    | rxo::retry()
                    // forget type to workaround lambda deduction bug on msvc 2013
                    | rxo::as_dynamic();
            }
            );

            THEN("the output contains all the data until complete") {
                auto required = rxu::to_vector({
                    on.next(300, 1),
                    on.next(325, 2),
                    on.next(350, 3),
                    on.next(425, 1),
                    on.next(450, 2),
                    on.next(475, 3),
                    on.next(500, 4),
                    on.next(550, 1),
                    on.next(575, 2),
                    on.next(600, 3),
                    on.next(625, 4),
                    on.next(650, 5),
                    on.next(675, 6),
                    on.next(700, 7),
                    on.completed(725)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("there were 3 subscriptions and 3 unsubscriptions to the ints") {
                auto required = rxu::to_vector({
                    on.subscribe(200, 400),
                    on.subscribe(400, 525),
                    on.subscribe(525, 725)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("retry 0, basic test", "[retry][operators]") {
  GIVEN("hot observable of 3 ints. Infinite retry.") {
    auto sc = rxsc::make_test();
    auto w = sc.create_worker();
    const rxsc::test::messages<int> on;
    std::runtime_error ex("retry on_error from source");

    auto xs = sc.make_hot_observable({
        on.next(100, 1),
          on.next(150, 2),
          on.next(200, 3),
          });;

    WHEN("retry is invoked with 0 times as argument") {

      auto res = w.start(
                         [&]() {
                           return xs
                           | rxo::retry(0)
                           // forget type to workaround lambda deduction bug on msvc 2013
                           | rxo::as_dynamic();
                         }
                         );
      
      THEN("the output should be empty"){
        auto required = rxu::to_vector({
            on.completed(200)
              });
        auto actual = res.get_observer().messages();
        REQUIRE(required == actual);
      }

      THEN("no subscriptions in retry(0)"){
        auto required = std::vector<rxcpp::notifications::subscription>();
        auto actual = xs.subscriptions();
        REQUIRE(required == actual);
      }

    }

  }
}
          

SCENARIO("retry with failure", "[retry][operators]") {
    GIVEN("hot observable of 3x4x7 ints with errors inbetween the groups. Retry 2. Must fail.") {
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;
        std::runtime_error ex("retry on_error from source");

        auto xs = sc.make_hot_observable({
            on.next(300, 1),
            on.next(325, 2),
            on.next(350, 3),
            on.error(400, ex),
            on.next(425, 1),
            on.next(450, 2),
            on.next(475, 3),
            on.next(500, 4),
            on.error(525, ex),
            on.next(550, 1),
            on.next(575, 2),
            on.next(600, 3),
            on.next(625, 4),
            on.next(650, 5),
            on.next(675, 6),
            on.next(700, 7),
            on.completed(725)
        });

        WHEN("retry of 2 is launched with expected error before complete") {

            auto res = w.start(
                [&]() {
                return xs
                    .retry(2)
                    // forget type to workaround lambda deduction bug on msvc 2013
                    .as_dynamic();
            });

            THEN("The output contains all the data until retry fails") {
                auto required = rxu::to_vector({
                    on.next(300, 1),
                    on.next(325, 2),
                    on.next(350, 3),
                    on.next(425, 1),
                    on.next(450, 2),
                    on.next(475, 3),
                    on.next(500, 4),
                    on.error(525, ex),
                });
                auto actual = res.get_observer().messages();
                REQUIRE(actual == required);
            }

            THEN("There were 2 subscriptions and 2 unsubscriptions to the ints") {
                auto required = rxu::to_vector({
                    on.subscribe(200, 400),
                    on.subscribe(400, 525)
                });
                auto actual = xs.subscriptions();
                REQUIRE(required == actual);
            }
        }
    }
}


