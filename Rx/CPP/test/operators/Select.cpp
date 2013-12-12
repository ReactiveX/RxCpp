#include "cpprx/rx.hpp"
namespace rx=rxcpp;

#include "catch.hpp"

SCENARIO("select throws", "[select][map][operators]"){
    GIVEN("select"){
        WHEN("subscribed to onnext that throws"){
            THEN("the exception is not supressed"){
                auto next_throws = [](int){
                    throw std::runtime_error("onnext throws"); };
                REQUIRE_THROWS(from(rx::Return(1))
                    .select([](int v){return v;})
                    .subscribe(next_throws));
            }
        }

        WHEN("subscribed to throw operator"){
            THEN("the exception is not supressed"){
                REQUIRE_THROWS(from(rx::Throw<int>(std::runtime_error("throw operator")))
                    .select([](int v){return v;})
                    .subscribe([](int){}, [](){}, [](std::exception_ptr){ throw std::runtime_error("onerror throws"); }));
            }
        }

        WHEN("subscribed to oncompleted that throws"){
            THEN("the exception is not supressed"){
                REQUIRE_THROWS(from(rx::Empty<int>())
                    .select([](int v){return v;})
                    .subscribe([](int){}, [](){ throw std::runtime_error("oncompleted throws"); }, [](std::exception_ptr){}));
            }
        }
    }
}

SCENARIO("select should throw", "[select][map][operators][hide]"){
    GIVEN("select"){
        WHEN("subscribe throws"){
            THEN("the exception is not supressed"){
                // not yet sure why this fails
                auto subscribe_throws = [](std::shared_ptr<rxcpp::Observer<int>> observer) -> rxcpp::Disposable {
                    throw std::runtime_error("subscribe throws"); };
                REQUIRE_THROWS(from(rx::CreateObservable<int>(subscribe_throws))
                    .select([](int v){return v;})
                    .subscribe([](int){}));
            }
        }
    }
}

SCENARIO("select stops on completion", "[select][map][operators]"){
    GIVEN("a test hot observable of ints"){
        auto scheduler = std::make_shared<rx::TestScheduler>();
        typedef rx::TestScheduler::Messages<int> m;

        long invoked = 0;

        auto xs = scheduler->CreateHotObservable(
            []() {
                m::RecordedT messages[] = {
                    m::OnNext(180, 1),
                    m::OnNext(210, 2),
                    m::OnNext(240, 3),
                    m::OnNext(290, 4),
                    m::OnNext(350, 5),
                    m::OnCompleted(400),
                    m::OnNext(410, -1),
                    m::OnCompleted(420),
                    m::OnError(430, std::exception())
                };
                return m::ToVector(messages);
            }()
            );

        WHEN("mapped to ints that are one larger"){

            auto res = scheduler->Start<int>(
                [xs, &invoked]() {
                    return rx::observable(rx::from(xs)
                        .select([&invoked](int x) {
                            invoked++;
                            return x + 1;
                        }));
                }
            );

            THEN("the output only contains primes"){
                m::RecordedT items[] = {
                    m::OnNext(210, 3),
                    m::OnNext(240, 4),
                    m::OnNext(290, 5),
                    m::OnNext(350, 6),
                    m::OnCompleted(400),
                };
                auto required = m::ToVector(items);
                auto actual = res->Messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                rx::Subscription items[] = {
                    m::Subscribe(200, 400)
                };
                auto required = m::ToVector(items);
                auto actual = xs->Subscriptions();
                REQUIRE(required == actual);
            }

            THEN("where was called until completed"){
                REQUIRE(4 == invoked);
            }
        }
    }
}

SCENARIO("select stops on disposal", "[select][map][operators]"){
    GIVEN("a test hot observable of ints"){
        auto scheduler = std::make_shared<rx::TestScheduler>();
        typedef rx::TestScheduler::Messages<int> m;

        long invoked = 0;

        auto xs = scheduler->CreateHotObservable(
            []() {
                m::RecordedT messages[] = {
                    m::OnNext(100, 1),
                    m::OnNext(200, 2),
                    m::OnNext(500, 3),
                    m::OnNext(600, 4)
                };
                return m::ToVector(messages);
            }()
            );

        auto res = scheduler->CreateObserver<int>();

        WHEN("the ints are identity mapped"){

            rx::SerialDisposable d;

            d.Set(observable(from(xs)
                .select([&](int x) {
                    invoked++;
                    if (scheduler->Clock() > 400) {
                        d.Dispose();
                    }
                    return x;
                }))
                ->Subscribe(observer(res))
            );

            scheduler->ScheduleAbsolute(rx::TestScheduler::Disposed, [&](rx::Scheduler::shared) {
                d.Dispose(); return rx::Disposable::Empty();});

            scheduler->Start();

            THEN("the output only contains values that arrived before disposal"){
                m::RecordedT items[] = {
                    m::OnNext(100, 1),
                    m::OnNext(200, 2)
                };
                auto required = m::ToVector(items);
                auto actual = res->Messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                rx::Subscription items[] = {
                    m::Subscribe(0, 500)
                };
                auto required = m::ToVector(items);
                auto actual = xs->Subscriptions();
                REQUIRE(required == actual);
            }

            THEN("select was called until disposed"){
                REQUIRE(3 == invoked);
            }
        }
    }
}
