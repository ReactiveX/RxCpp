#include "cpprx/rx.hpp"
namespace rx=rxcpp;

#include "catch.hpp"

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

SCENARIO("where stops on completion", "[where][filter][operators]"){
    GIVEN("a test hot observable of longs"){
        auto scheduler = std::make_shared<rx::TestScheduler>();
        typedef rx::TestScheduler::Messages<long> m;

        long invoked = 0;

        auto xs = scheduler->CreateHotObservable(
            []() { 
                m::RecordedT messages[] = {
                    m::OnNext(110, 1),
                    m::OnNext(180, 2),
                    m::OnNext(230, 3),
                    m::OnNext(270, 4),
                    m::OnNext(340, 5),
                    m::OnNext(380, 6),
                    m::OnNext(390, 7),
                    m::OnNext(450, 8),
                    m::OnNext(470, 9),
                    m::OnNext(560, 10),
                    m::OnNext(580, 11),
                    m::OnCompleted(600),
                    m::OnNext(610, 12),
                    m::OnError(620, std::exception()),
                    m::OnCompleted(630)
                };
                return m::ToVector(messages);
            }()
            );

        WHEN("filtered to longs that are primes"){

            auto res = scheduler->Start<long>(
                [xs, &invoked]() {
                    return rx::observable(rx::from(xs)
                        .where([&invoked](long x) {
                            invoked++;
                            return IsPrime(x);
                        }));
                }
            );

            THEN("the output only contains primes"){
                m::RecordedT items[] = {
                    m::OnNext(230, 3),
                    m::OnNext(340, 5),
                    m::OnNext(390, 7),
                    m::OnNext(580, 11),
                    m::OnCompleted(600)
                };
                auto required = m::ToVector(items);
                auto actual = res->Messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                rx::Subscription items[] = {
                    m::Subscribe(200, 600)
                };
                auto required = m::ToVector(items);
                auto actual = xs->Subscriptions();
                REQUIRE(required == actual);
            }

            THEN("where was called until completed"){
                REQUIRE(9 == invoked);
            }
        }
    }
}

SCENARIO("where stops on disposal", "[where][filter][operators]"){
    GIVEN("a test hot observable of longs"){
        auto scheduler = std::make_shared<rx::TestScheduler>();
        typedef rx::TestScheduler::Messages<long> m;

        long invoked = 0;

        auto xs = scheduler->CreateHotObservable(
            []() { 
                m::RecordedT messages[] = {
                    m::OnNext(110, 1),
                    m::OnNext(180, 2),
                    m::OnNext(230, 3),
                    m::OnNext(270, 4),
                    m::OnNext(340, 5),
                    m::OnNext(380, 6),
                    m::OnNext(390, 7),
                    m::OnNext(450, 8),
                    m::OnNext(470, 9),
                    m::OnNext(560, 10),
                    m::OnNext(580, 11),
                    m::OnCompleted(600)
                };
                return m::ToVector(messages);
            }()
            );

        WHEN("filtered to longs that are primes"){

            auto res = scheduler->Start<long>(
                [xs, &invoked]() {
                    return rx::observable(rx::from(xs)
                        .where([&invoked](long x) {
                            invoked++;
                            return IsPrime(x);
                        }));
                },
                400
            );

            THEN("the output only contains primes that arrived before disposal"){
                m::RecordedT items[] = {
                    m::OnNext(230, 3),
                    m::OnNext(340, 5),
                    m::OnNext(390, 7)
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

            THEN("where was called until disposed"){
                REQUIRE(5 == invoked);
            }
        }
    }
}

SCENARIO("where stops on error", "[where][filter][operators]"){
    GIVEN("a test hot observable of longs"){
        auto scheduler = std::make_shared<rx::TestScheduler>();
        typedef rx::TestScheduler::Messages<long> m;

        long invoked = 0;

        std::exception ex;

        auto xs = scheduler->CreateHotObservable(
            [ex]() { 
                m::RecordedT messages[] = {
                    m::OnNext(110, 1),
                    m::OnNext(180, 2),
                    m::OnNext(230, 3),
                    m::OnNext(270, 4),
                    m::OnNext(340, 5),
                    m::OnNext(380, 6),
                    m::OnNext(390, 7),
                    m::OnNext(450, 8),
                    m::OnNext(470, 9),
                    m::OnNext(560, 10),
                    m::OnNext(580, 11),
                    m::OnError(600, ex),
                    m::OnNext(610, 12),
                    m::OnError(620, std::exception()),
                    m::OnCompleted(630)
                };
                return m::ToVector(messages);
            }()
            );

        WHEN("filtered to longs that are primes"){

            auto res = scheduler->Start<long>(
                [xs, &invoked]() {
                    return rx::observable(rx::from(xs)
                        .where([&invoked](long x) {
                            invoked++;
                            return IsPrime(x);
                        }));
                }
            );

            THEN("the output only contains primes"){
                m::RecordedT items[] = {
                    m::OnNext(230, 3),
                    m::OnNext(340, 5),
                    m::OnNext(390, 7),
                    m::OnNext(580, 11),
                    m::OnError(600, ex),
                };
                auto required = m::ToVector(items);
                auto actual = res->Messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                rx::Subscription items[] = {
                    m::Subscribe(200, 600)
                };
                auto required = m::ToVector(items);
                auto actual = xs->Subscriptions();
                REQUIRE(required == actual);
            }

            THEN("where was called until error"){
                REQUIRE(9 == invoked);
            }
        }
    }
}

SCENARIO("where stops on throw from predicate", "[where][filter][operators]"){
    GIVEN("a test hot observable of longs"){
        auto scheduler = std::make_shared<rx::TestScheduler>();
        typedef rx::TestScheduler::Messages<long> m;

        long invoked = 0;

        std::exception ex;

        auto xs = scheduler->CreateHotObservable(
            []() { 
                m::RecordedT messages[] = {
                    m::OnNext(110, 1),
                    m::OnNext(180, 2),
                    m::OnNext(230, 3),
                    m::OnNext(270, 4),
                    m::OnNext(340, 5),
                    m::OnNext(380, 6),
                    m::OnNext(390, 7),
                    m::OnNext(450, 8),
                    m::OnNext(470, 9),
                    m::OnNext(560, 10),
                    m::OnNext(580, 11),
                    m::OnCompleted(600),
                    m::OnNext(610, 12),
                    m::OnError(620, std::exception()),
                    m::OnCompleted(630)
                };
                return m::ToVector(messages);
            }()
            );

        WHEN("filtered to longs that are primes"){

            auto res = scheduler->Start<long>(
                [ex, xs, &invoked]() {
                    return rx::observable(rx::from(xs)
                        .where([ex, &invoked](long x) {
                            invoked++;
                            if (x > 5) {
                                throw ex;
                            }
                            return IsPrime(x);
                        }));
                }
            );

            THEN("the output only contains primes"){
                m::RecordedT items[] = {
                    m::OnNext(230, 3),
                    m::OnNext(340, 5),
                    m::OnError(380, ex)
                };
                auto required = m::ToVector(items);
                auto actual = res->Messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                rx::Subscription items[] = {
                    m::Subscribe(200, 380)
                };
                auto required = m::ToVector(items);
                auto actual = xs->Subscriptions();
                REQUIRE(required == actual);
            }

            THEN("where was called until error"){
                REQUIRE(4 == invoked);
            }
        }
    }
}

SCENARIO("where stops on dispose from predicate", "[where][filter][operators]"){
    GIVEN("a test hot observable of longs"){
        auto scheduler = std::make_shared<rx::TestScheduler>();
        typedef rx::TestScheduler::Messages<long> m;

        long invoked = 0;

        auto xs = scheduler->CreateHotObservable(
            []() { 
                m::RecordedT messages[] = {
                    m::OnNext(110, 1),
                    m::OnNext(180, 2),
                    m::OnNext(230, 3),
                    m::OnNext(270, 4),
                    m::OnNext(340, 5),
                    m::OnNext(380, 6),
                    m::OnNext(390, 7),
                    m::OnNext(450, 8),
                    m::OnNext(470, 9),
                    m::OnNext(560, 10),
                    m::OnNext(580, 11),
                    m::OnCompleted(600),
                    m::OnNext(610, 12),
                    m::OnError(620, std::exception()),
                    m::OnCompleted(630)
                };
                return m::ToVector(messages);
            }()
        );

        auto res = scheduler->CreateObserver<long>();

        rx::SerialDisposable d;
        std::shared_ptr<rx::Observable<long>> ys;

        WHEN("filtered to longs that are primes"){

            scheduler->ScheduleAbsolute(rx::TestScheduler::Created, 
                [&invoked, &d, &ys, &xs](rx::Scheduler::shared) { 
                    ys = rx::observable(rx::from(xs)
                        .where([&invoked, &d](long x) {
                            invoked++;
                            if (x == 8)
                                d.Dispose();
                            return IsPrime(x);
                        }));
                    return rx::Disposable::Empty();
                });

            scheduler->ScheduleAbsolute(rx::TestScheduler::Subscribed, [&d, &ys, &res](rx::Scheduler::shared) { 
                d.Set(ys->Subscribe(res));
                return rx::Disposable::Empty();
            });

            scheduler->ScheduleAbsolute(rx::TestScheduler::Disposed, [&d](rx::Scheduler::shared) { 
                d.Dispose(); 
                return rx::Disposable::Empty();
            });

            scheduler->Start();

            THEN("the output only contains primes"){
                m::RecordedT items[] = {
                    m::OnNext(230, 3),
                    m::OnNext(340, 5),
                    m::OnNext(390, 7)
                };
                auto required = m::ToVector(items);
                auto actual = res->Messages();
                REQUIRE(required == actual);
            }

            THEN("there was one subscription and one unsubscription"){
                rx::Subscription items[] = {
                    m::Subscribe(200, 450)
                };
                auto required = m::ToVector(items);
                auto actual = xs->Subscriptions();
                REQUIRE(required == actual);
            }

            THEN("where was called until disposed"){
                REQUIRE(6 == invoked);
            }
        }
    }
}

