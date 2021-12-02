#include "../test.h"

SCENARIO("create stops on completion", "[create][sources]"){
    GIVEN("a test cold observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        long invoked = 0;

        WHEN("created"){

            auto res = w.start(
                [&]() {
                    return rx::observable<>::create<int>(
                        [&](const rx::subscriber<int>& s){
                            invoked++;
                            s.on_next(1);
                            s.on_next(2);
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains all items"){
                auto required = rxu::to_vector({
                    on.next(200, 1),
                    on.next(200, 2)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("create was called until completed"){
                REQUIRE(1 == invoked);
            }
        }
    }
}

SCENARIO("when observer::on_next is overridden", "[create][observer][sources]"){
    GIVEN("a test cold observable of ints"){
        auto sc = rxsc::make_test();
        auto w = sc.create_worker();
        const rxsc::test::messages<int> on;

        long invoked = 0;

        WHEN("created"){

            auto res = w.start(
                [&]() {
                    return rx::observable<>::create<int>(
                        [&](const rx::subscriber<int>& so){
                            invoked++;
                            auto sn = rx::make_subscriber<int>(so,
                                rx::make_observer<int>(so.get_observer(),
                                    [](rx::observer<int>& o, int v){
                                        o.on_next(v + 1);
                                    }));
                            sn.on_next(1);
                            sn.on_next(2);
                        })
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();
                }
            );

            THEN("the output contains all items incremented by 1"){
                auto required = rxu::to_vector({
                    on.next(200, 2),
                    on.next(200, 3)
                });
                auto actual = res.get_observer().messages();
                REQUIRE(required == actual);
            }

            THEN("create was called until completed"){
                REQUIRE(1 == invoked);
            }
        }
    }
}


SCENARIO("create doesn't provide copies", "[create][sources][copies]")
{
    GIVEN("observable and subscriber")
    {
        auto          empty_on_next = [](copy_verifier) {};
        auto          sub           = rx::make_observer<copy_verifier>(empty_on_next);
        copy_verifier verifier{};
        auto          obs = rxcpp::observable<>::create<copy_verifier>([&verifier](rxcpp::subscriber<copy_verifier> sub)
        {
            sub.on_next(verifier);
            sub.on_completed();
        });

        WHEN("subscribe")
        {
            obs.subscribe(sub);
            THEN("no extra copies")
            {
                // 1 copy to final lambda
                REQUIRE(verifier.get_copy_count() == 1);
                REQUIRE(verifier.get_move_count() == 0);
            }
        }
    }
}


SCENARIO("create doesn't provide copies for move", "[create][sources][copies]")
{
    GIVEN("observable and subscriber")
    {
        auto          empty_on_next = [](copy_verifier) {};
        auto          sub           = rx::make_observer<copy_verifier>(empty_on_next);
        copy_verifier verifier{};
        auto          obs = rxcpp::observable<>::create<copy_verifier>([&verifier](rxcpp::subscriber<copy_verifier> sub)
        {
            sub.on_next(std::move(verifier));
            sub.on_completed();
        });
        WHEN("subscribe")
        {
            obs.subscribe(sub);
            THEN("no extra copies")
            {
                REQUIRE(verifier.get_copy_count() == 0);
                //  1 move to final lambda
                REQUIRE(verifier.get_move_count() == 1);
            }
        }
    }
}
