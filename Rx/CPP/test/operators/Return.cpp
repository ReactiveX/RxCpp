#include "cpprx/rx.hpp"
namespace rx=rxcpp;

#include "catch.hpp"

SCENARIO("return basic", "[return][operators]"){
    GIVEN("return 42"){
        auto scheduler = std::make_shared<rx::TestScheduler>();
        typedef rx::TestScheduler::Messages<int> m;

        auto res = scheduler->Start<int>(
            [=]() { 
                return rx::Return(42, scheduler);
            }
        );

        WHEN("started"){

            THEN("the output is 42"){
                m::RecordedT items[] = {
                    m::OnNext(201, 42),
                    m::OnCompleted(201)
                };
                auto required = m::ToVector(items);
                auto actual = res->Messages();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("return disposed", "[return][operators]"){
    GIVEN("test scheduler"){
        auto scheduler = std::make_shared<rx::TestScheduler>();
        typedef rx::TestScheduler::Messages<int> m;

        WHEN("return 42 after disposed"){
            auto res = scheduler->Start<int>(
                [&]() { 
                    return rx::Return(42, scheduler);
                },
                200
            );

            THEN("the output is empty"){
                std::vector<m::RecordedT> required;
                auto actual = res->Messages();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("return disposed after next", "[return][operators]"){
    GIVEN("return 42 after disposal"){
        auto scheduler = std::make_shared<rx::TestScheduler>();
        typedef rx::TestScheduler::Messages<int> m;

        rx::SerialDisposable d;

        auto xs = rx::Return(42, scheduler);

        auto res = scheduler->CreateObserver<int>();

        scheduler->ScheduleAbsolute(
            100,
            [&](rx::Scheduler::shared) {
                d.Set(from(xs).subscribe(
                    [&](int x){
                        d.Dispose(); res->OnNext(x);},
                    [&](){
                        res->OnCompleted();},
                    [&](std::exception_ptr ex){
                        res->OnError(ex);}));
                return d;
            }
        );

        WHEN("started"){

            scheduler->Start();

            THEN("the output is 42"){
                m::RecordedT items[] = {
                    m::OnNext(101, 42)
                };
                auto required = m::ToVector(items);
                auto actual = res->Messages();
                REQUIRE(required == actual);
            }
        }
    }
}

SCENARIO("return observer throws", "[return][operators]"){
    GIVEN("return 42"){
        auto scheduler = std::make_shared<rx::TestScheduler>();
        typedef rx::TestScheduler::Messages<int> m;

        auto xs = rx::Return(42, scheduler);

        WHEN("subscribed to onnext that throws"){
            from(xs).subscribe([](int){ throw std::runtime_error("onnext throws"); });

            THEN("the exception is not supressed"){
                REQUIRE_THROWS(scheduler->Start());
            }
        }

        WHEN("subscribed to oncompleted that throws"){
            from(xs).subscribe([](int){},[](){ throw std::runtime_error("oncompleted throws"); });

            THEN("the exception is not supressed"){
                REQUIRE_THROWS(scheduler->Start());
            }
        }
    }
}

