#include "cpprx/rx.hpp"
namespace rx=rxcpp;

#include "catch.hpp"

SCENARIO("publish_last", "[publish_last][publish][multicast][operators]"){
    GIVEN("a test hot observable of longs"){
        auto scheduler = std::make_shared<rx::TestScheduler>();
        typedef rx::TestScheduler::Messages<long> m;

        long invoked = 0;

        auto xs = scheduler->CreateHotObservable(
            []() { 
                m::RecordedT messages[] = {
                    m::OnNext(110, 7),
                    m::OnNext(220, 3),
                    m::OnNext(280, 4),
                    m::OnNext(290, 1),
                    m::OnNext(340, 8),
                    m::OnNext(360, 5),
                    m::OnNext(370, 6),
                    m::OnNext(390, 7),
                    m::OnNext(410, 13),
                    m::OnNext(430, 2),
                    m::OnNext(450, 9),
                    m::OnNext(520, 11),
                    m::OnNext(560, 20),
                    m::OnCompleted(600)
                };
                return m::ToVector(messages);
            }()
        );

        auto res = scheduler->CreateObserver<long>();

        rx::SerialDisposable subscription;
        std::shared_ptr<rx::ConnectableObservable<long>> ys;

        WHEN("subscribed and then connected"){

            scheduler->ScheduleAbsolute(rx::TestScheduler::Created, 
                [&invoked, &ys, &xs](rx::Scheduler::shared) { 
                    ys = rx::observable(rx::from(xs)
                        .publish_last());
                    return rx::Disposable::Empty();
                });

            scheduler->ScheduleAbsolute(rx::TestScheduler::Subscribed, [&subscription, &ys, &res](rx::Scheduler::shared) { 
                subscription.Set(ys->Subscribe(res));
                return rx::Disposable::Empty();
            });

            scheduler->ScheduleAbsolute(rx::TestScheduler::Disposed, [&subscription](rx::Scheduler::shared) { 
                subscription.Dispose(); 
                return rx::Disposable::Empty();
            });

            auto connection = std::make_shared<rx::SerialDisposable>();
            scheduler->ScheduleAbsolute(300, [connection, &ys](rx::Scheduler::shared) { 
                connection->Set(ys->Connect());
                return rx::Disposable::Empty();
            });
            scheduler->ScheduleAbsolute(400, [connection](rx::Scheduler::shared) { 
                connection->Dispose();
                return rx::Disposable::Empty();
            });

            connection = std::make_shared<rx::SerialDisposable>();
            scheduler->ScheduleAbsolute(500, [connection, &ys](rx::Scheduler::shared) { 
                connection->Set(ys->Connect());
                return rx::Disposable::Empty();
            });
            scheduler->ScheduleAbsolute(550, [connection](rx::Scheduler::shared) { 
                connection->Dispose();
                return rx::Disposable::Empty();
            });

            connection = std::make_shared<rx::SerialDisposable>();
            scheduler->ScheduleAbsolute(650, [connection, &ys](rx::Scheduler::shared) { 
                connection->Set(ys->Connect());
                return rx::Disposable::Empty();
            });
            scheduler->ScheduleAbsolute(800, [connection](rx::Scheduler::shared) { 
                connection->Dispose();
                return rx::Disposable::Empty();
            });

            scheduler->Start();

            THEN("the output is empty"){
                std::vector<m::RecordedT> required;
                auto actual = res->Messages();
                REQUIRE(required == actual);
            }

            THEN("there were 3 subscription/unsubscription"){
                rx::Subscription items[] = {
                    m::Subscribe(300, 400),
                    m::Subscribe(500, 550),
                    m::Subscribe(650, 800)
                };
                auto required = m::ToVector(items);
                auto actual = xs->Subscriptions();
                REQUIRE(required == actual);
            }

        }
    }
}

