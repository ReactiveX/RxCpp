#include "../test.h"

SCENARIO("just doesn't provide copies", "[just][sources][copies]")
{
    GIVEN("observable and subscriber")
    {
        auto          empty_on_next = [](copy_verifier) {};
        auto          sub           = rx::make_observer<copy_verifier>(empty_on_next);
        copy_verifier verifier{};
        auto          obs = rxcpp::observable<>::just(verifier);

        WHEN("subscribe")
        {
            obs.subscribe(sub);
            THEN("no extra copies")
            {
                // 1 copy to collection  + 1 copy to final lambda
                CHECK(verifier.get_copy_count() == 2);
                // 1 move to internal state
                CHECK(verifier.get_move_count() == 1);
            }
        }
    }
}


SCENARIO("just doesn't provide copies for move", "[just][sources][copies]")
{
    GIVEN("observable and subscriber")
    {
        auto          empty_on_next = [](copy_verifier) {};
        auto          sub           = rx::make_observer<copy_verifier>(empty_on_next);
        copy_verifier verifier{};
        auto          obs = rxcpp::observable<>::just(std::move(verifier));;
        WHEN("subscribe")
        {
            obs.subscribe(sub);
            THEN("no extra copies")
            {
                // 1 copy to final lambda
                CHECK(verifier.get_copy_count() == 1);
                // 1 move to collection  +  1 move to internal state
                CHECK(verifier.get_move_count() == 2);
            }
        }
    }
}

SCENARIO("from variadic doesn't provide copies", "[from][sources][copies]")
{
    GIVEN("observable and subscriber")
    {
        auto          empty_on_next = [](copy_verifier) {};
        auto          sub           = rx::make_observer<copy_verifier>(empty_on_next);
        copy_verifier verifier{};
        auto          obs = rxcpp::observable<>::from(verifier, verifier, verifier);

        WHEN("subscribe")
        {
            obs.subscribe(sub);
            THEN("no extra copies")
            {
                // 1 copy to collection  + 1 copy to final lambda
                CHECK(verifier.get_copy_count() == 3*2);
                // 1 move to internal state
                CHECK(verifier.get_move_count() == 3*1);
            }
        }
    }
}


SCENARIO("from variadic doesn't provide copies for move", "[from][sources][copies]")
{
    GIVEN("observable and subscriber")
    {
        auto          empty_on_next = [](copy_verifier) {};
        auto          sub           = rx::make_observer<copy_verifier>(empty_on_next);
        copy_verifier verifier{};
        auto          obs = rxcpp::observable<>::from(std::move(verifier), std::move(verifier), std::move(verifier));;
        WHEN("subscribe")
        {
            obs.subscribe(sub);
            THEN("no extra copies")
            {
                // 1 copy to final lambda
                CHECK(verifier.get_copy_count() == 3*1);
                // 1 move to collection + 1 move to internal state
                CHECK(verifier.get_move_count() == 3*2);
            }
        }
    }
}

SCENARIO("iterate doesn't provide copies", "[iterate][sources][copies]")
{
    GIVEN("observable and subscriber")
    {
        auto          empty_on_next = [](copy_verifier) {};
        auto          sub           = rx::make_observer<copy_verifier>(empty_on_next);
        copy_verifier verifier{};
        std::vector<copy_verifier> vec{verifier};
        int                        initial_copies = verifier.get_copy_count();
        int                        initial_moves  = verifier.get_move_count();
        auto                       obs            = rxcpp::observable<>::iterate(vec);

        WHEN("subscribe")
        {
            obs.subscribe(sub);
            THEN("no extra copies")
            {
                // 1 copy to internal state + 1 copy to final lambda
                CHECK(verifier.get_copy_count() - initial_copies == 2);
                CHECK(verifier.get_move_count() - initial_moves  == 0);
            }
        }
    }
}


SCENARIO("iterate doesn't provide copies for move", "[iterate][sources][copies]")
{
    GIVEN("observable and subscriber")
    {
        auto          empty_on_next = [](copy_verifier) {};
        auto          sub           = rx::make_observer<copy_verifier>(empty_on_next);
        copy_verifier verifier{};
        std::vector<copy_verifier> vec{std::move(verifier)};
        int                        initial_copies = verifier.get_copy_count();
        int                        initial_moves = verifier.get_move_count();
        auto          obs = rxcpp::observable<>::iterate(std::move(vec));
        WHEN("subscribe")
        {
            obs.subscribe(sub);
            THEN("no extra copies")
            {
                //  1 copy to final lambda
                CHECK(verifier.get_copy_count() - initial_copies == 1);
                CHECK(verifier.get_move_count() - initial_moves == 0);
            }
        }
    }
}
