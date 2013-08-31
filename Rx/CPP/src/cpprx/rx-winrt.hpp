#pragma once
#include "rx-includes.hpp"

#if !defined(CPPRX_RX_WINRT_HPP)
#define CPPRX_RX_WINRT_HPP
#pragma once

#if RXCPP_USE_WINRT

#define NOMINMAX
#include <Windows.h>

namespace rxcpp { namespace winrt {
    namespace wf = Windows::Foundation;
    namespace wuicore = Windows::UI::Core;
    namespace wuixaml = Windows::UI::Xaml;
    namespace wuictrls = Windows::UI::Xaml::Controls;

    template <class TSender, class TEventArgs>
    struct EventPattern
    {
        EventPattern(TSender sender, TEventArgs eventargs) :
            sender(sender),
            eventargs(eventargs)
        {}

        TSender Sender() const {
            return sender;};
        TEventArgs EventArgs() const {
            return eventargs;};

    private:
        TSender sender;
        TEventArgs eventargs;
    };

    namespace detail
    {
        template <class SpecificTypedEventHandler>
        struct is_typed_event_handler : public std::false_type {};

        template <class Sender, class EventArgs>
        struct is_typed_event_handler<wf::TypedEventHandler<Sender, EventArgs>> : public std::true_type {};

        template <class SpecificTypedEventHandler>
        struct get_sender;
        template <class Sender, class EventArgs>
        struct get_sender<wf::TypedEventHandler<Sender, EventArgs>>
        {
            typedef Sender type;
        };

        template <class SpecificTypedEventHandler>
        struct get_eventargs;
        template <class Sender, class EventArgs>
        struct get_eventargs<wf::TypedEventHandler<Sender, EventArgs>>
        {
            typedef EventArgs type;
        };

        template <class SpecificTypedEventHandler>
        struct get_eventpattern;
        template <class Sender, class EventArgs>
        struct get_eventpattern<wf::TypedEventHandler<Sender, EventArgs>>
        {
            typedef EventPattern<Sender, EventArgs> type;
        };

        template<class T>
        struct remove_ref { typedef T type; };
        template<class T>
        struct remove_ref<T^> { typedef T type; };
        template<class T>
        struct remove_ref<T^ const> { typedef T type; };
        template<class T>
        struct remove_ref<T^ const &> { typedef T type; };

        template<typename Result>
        wf::IAsyncOperation<Result>^ operation_interface(wf::IAsyncOperation<Result>^ i) { return i; }

        template<typename Result, typename Progress>
        wf::IAsyncOperationWithProgress<Result, Progress>^ operation_interface(wf::IAsyncOperationWithProgress<Result, Progress>^ i) { return i; }
    }

    template <class EventHandler, class EventArgs>
    auto FromEventPattern(
        std::function<wf::EventRegistrationToken(EventHandler^)> addHandler,
        std::function<void (wf::EventRegistrationToken)> removeHandler)
        -> typename std::enable_if < !detail::is_typed_event_handler<EventHandler>::value, std::shared_ptr < Observable < EventPattern < Platform::Object^, EventArgs^ >> >> ::type
    {
        typedef EventPattern<Platform::Object^, EventArgs^> EP;
        return CreateObservable<EP>(
            [=](std::shared_ptr<Observer<EP>> observer)
            {
                auto h = ref new EventHandler(
                    [=](Platform::Object^ sender, EventArgs^ args) -> void
                    {
                        observer->OnNext(EP(sender, args));
                    });

                auto token = addHandler(h);

                return Disposable(
                    [removeHandler, token]()
                {
                    removeHandler(token);
                });
        });
    }

    template <class EventHandler, class Sender, class EventArgs>
    auto FromEventPattern(
        std::function<wf::EventRegistrationToken(EventHandler^)> addHandler,
        std::function<void (wf::EventRegistrationToken)> removeHandler)
        -> typename std::enable_if < !detail::is_typed_event_handler<EventHandler>::value, std::shared_ptr < Observable < EventPattern < Sender^, EventArgs^ >> >> ::type
    {
        typedef EventPattern<Sender^, EventArgs^> EP;
        return CreateObservable<EP>(
            [=](std::shared_ptr<Observer<EP>> observer)
            {
                auto h = ref new EventHandler(
                    [=](Platform::Object^ sender, EventArgs^ args) -> void
                    {
                        observer->OnNext(EP(dynamic_cast<Sender^>(sender), args));
                    });

                auto token = addHandler(h);

                return Disposable(
                    [removeHandler, token]()
                {
                    removeHandler(token);
                });
        });
    }

    template <class SpecificTypedEventHandler>
    auto FromEventPattern(
        std::function<wf::EventRegistrationToken(SpecificTypedEventHandler^)> addHandler,
        std::function<void (wf::EventRegistrationToken)> removeHandler)
        -> typename std::enable_if < detail::is_typed_event_handler<SpecificTypedEventHandler>::value, std::shared_ptr < Observable < typename detail::get_eventpattern<SpecificTypedEventHandler>::type> >> ::type
    {
        typedef typename detail::get_eventpattern<SpecificTypedEventHandler>::type EP;
        return CreateObservable<EP>(
            [=](std::shared_ptr < Observer < EP >> observer)
        {
            auto h = ref new SpecificTypedEventHandler(
                [=](typename detail::get_sender<SpecificTypedEventHandler>::type sender, typename detail::get_eventargs<SpecificTypedEventHandler>::type args) -> void
                {
                    observer->OnNext(EP(sender, args));
                });

            auto token = addHandler(h);

            return Disposable(
                [removeHandler, token]()
                {
                    removeHandler(token);
                });
        });
    }

    template<typename... T, typename F>
    auto FromAsyncPattern(F&& start)
        -> std::function < std::shared_ptr < Observable< decltype(start((*(T*)nullptr)...)->GetResults()) >> (const T&...)>
    {
        typedef                                          decltype(start((*(T*)nullptr)...)->GetResults()) Result;

        return [=](const T&... t)
        {
            auto subject = CreateAsyncSubject<Result>();
            auto o = start(t...);
            typedef typename detail::remove_ref< decltype(o->Completed)>::type Handler;
            typedef decltype(detail::operation_interface(o)) Interface;
            o->Completed = ref new Handler([=](Interface io, wf::AsyncStatus)
            {
                util::maybe<Result> value;
                try
                {
                    value.set(io->GetResults());
                }
                catch (...)
                {
                    subject->OnError(std::current_exception());
                    return;
                }
                subject->OnNext(*value.get());
                subject->OnCompleted();
            });
            return observable(subject);
        };
    }

    std::shared_ptr < Observable < size_t> >
    inline DispatcherInterval(
        Scheduler::clock::duration interval)
    {
        return CreateObservable<size_t>(
            [=](std::shared_ptr < Observer < size_t >> observer)
            -> Disposable
        {
            size_t cursor = 0;
            ComposableDisposable cd;

            wf::TimeSpan timeSpan;
            // convert to 100ns ticks
            timeSpan.Duration = static_cast<int32>(std::chrono::duration_cast<std::chrono::nanoseconds>(interval).count() / 100);

            auto dispatcherTimer = ref new wuixaml::DispatcherTimer();
            dispatcherTimer->Interval = timeSpan;

            cd.Add(Subscribe(FromEventPattern<wf::EventHandler<Platform::Object^>, Platform::Object>(
                [dispatcherTimer](wf::EventHandler<Platform::Object^>^ h) {
                    return dispatcherTimer->Tick += h; },
                [dispatcherTimer](wf::EventRegistrationToken t) {
                    dispatcherTimer->Tick -= t; 
                }),
                [observer, cursor](EventPattern<Platform::Object^, Platform::Object^>) mutable {
                    observer->OnNext(cursor);
                    ++cursor;
                },
                [observer]() {
                    observer->OnCompleted();
                },
                [observer](std::exception_ptr e) {
                    observer->OnError(e);
                }));

            cd.Add(Disposable(
                [observer, dispatcherTimer](){
                    dispatcherTimer->Stop();
                    observer->OnCompleted();
                }));

            dispatcherTimer->Start();

            return cd;
        });
    }

    struct CoreDispatcherScheduler : public LocalScheduler
    {
    private:
        CoreDispatcherScheduler(const CoreDispatcherScheduler&);

    public:
        CoreDispatcherScheduler(wuicore::CoreDispatcher^ dispatcher, wuicore::CoreDispatcherPriority priority = wuicore::CoreDispatcherPriority::Normal)
            : dispatcher(dispatcher)
            , priority(priority)
        {
        }
        virtual ~CoreDispatcherScheduler()
        {
        }

        typedef std::shared_ptr<CoreDispatcherScheduler> shared;

        static shared Current()
        {
            auto window = wuixaml::Window::Current;
            if (window == nullptr)
            {
                throw std::logic_error("No window current");
            }
            auto d = window->Dispatcher;
            if (d == nullptr)
            {
                throw std::logic_error("No dispatcher on current window");
            }
            return std::make_shared<CoreDispatcherScheduler>(d);
        }

        wuicore::CoreDispatcher^ Dispatcher() 
        {
            return dispatcher;
        }

        wuicore::CoreDispatcherPriority Priority()
        {
            return priority;
        }

        using LocalScheduler::Schedule;
        virtual Disposable Schedule(clock::time_point dueTime, Work work)
        {
            auto that = shared_from_this();
            auto dispatchAsync = [this, that, work](Scheduler::shared sched) mutable -> Disposable
            {
                dispatcher->RunAsync(
                    priority,
                    ref new wuicore::DispatchedHandler(
                        [that, this, work]() mutable
                        {
                            this->Do(work, that);
                        },
                        Platform::CallbackContext::Any
                    ));
                return Disposable::Empty();
            };

            auto now = Now();
            auto interval = dueTime - now;
            if (now > dueTime || interval < std::chrono::milliseconds(10))
            {
                return dispatchAsync(nullptr);
            }

            wf::TimeSpan timeSpan;
            // convert to 100ns ticks
            timeSpan.Duration = static_cast<int32>(std::chrono::duration_cast<std::chrono::nanoseconds>(interval).count() / 100);

            auto dispatcherTimer = ref new wuixaml::DispatcherTimer();
            // convert to 100ns ticks
            dispatcherTimer->Interval = timeSpan;

            auto result = Subscribe(FromEventPattern<wf::EventHandler<Platform::Object^>, Platform::Object>(
                [dispatcherTimer](wf::EventHandler<Platform::Object^>^ h) {
                    return dispatcherTimer->Tick += h; },
                [dispatcherTimer](wf::EventRegistrationToken t) {
                    dispatcherTimer->Tick -= t;
                }),
                [dispatchAsync, dispatcherTimer](EventPattern<Platform::Object^, Platform::Object^>) mutable {
                    dispatcherTimer->Stop();
                    dispatchAsync(nullptr);
                });

            dispatcherTimer->Start();

            return result;
        }

    private:
        wuicore::CoreDispatcher^ dispatcher;
        wuicore::CoreDispatcherPriority priority;
    };

    template<class T>
    class ReactiveCommand : public Observable<T>
    {
        typedef ReactiveCommand<T> This;

        std::mutex flight_lock;
        std::shared_ptr < Subject<bool>> inflight;
        std::shared_ptr < Subject<std::exception_ptr>> exceptions;
        std::shared_ptr<Subject<T>> executed;
        Scheduler::shared defaultScheduler;
        bool allowsConcurrentExecution;
        std::shared_ptr < Observable < bool >> isExecuting;
        std::shared_ptr < Observable < bool >> canExecuteObservable;

    public:
        typedef std::shared_ptr<This> shared;

        ReactiveCommand(std::shared_ptr < Observable < bool >> canExecute = nullptr, bool allowsConcurrentExecution = false, Scheduler::shared scheduler = nullptr, bool initialCondition = true) :
            inflight(std::make_shared < Subject < bool >> ()),
            exceptions(std::make_shared < Subject <std::exception_ptr>>()),
            executed(std::make_shared < Subject < T >> ()),
            defaultScheduler(scheduler),
            allowsConcurrentExecution(allowsConcurrentExecution)
        {
            if (!canExecute)
            {
                canExecute = Return(true);
            }
            if (!defaultScheduler)
            {
                defaultScheduler = std::static_pointer_cast<Scheduler>(winrt::CoreDispatcherScheduler::Current());
            }

            isExecuting = observable(from(inflight)
                .scan(0, [](int balanced, bool in) { 
                    return in ? balanced + 1 : balanced - 1; })
                .select([](int balanced) { 
                    return balanced > 0; })
                .publish(false)
                .connect_forever()
                .distinct_until_changed());

            auto isBusy = allowsConcurrentExecution ? Return(false) : isExecuting;
            auto canExecuteAndNotBusy = from(isBusy)
                .combine_latest([](bool b, bool ce)
                {
                    return ce && !b; 
                }, canExecute);

            auto canExecuteObs = from(canExecuteAndNotBusy)
                .publish(initialCondition)
                .ref_count();

            canExecuteObservable = observable(from(canExecuteObs)
                .distinct_until_changed()
                .observe_on(defaultScheduler));
        }

        template<class R>
        std::shared_ptr<Observable<R>> RegisterAsync(std::function<std::shared_ptr<Observable<R>>(T)> f)
        {
            return observable(from(executed)
                .select_many(
                // select collection
                [=](T t) -> std::shared_ptr<Observable<R>>
                {
                    return Using<R, SerialDisposable>(
                    // resource factory
                    [=]() -> SerialDisposable
                    {
                        std::unique_lock<std::mutex> guard(flight_lock);
                        inflight->OnNext(true);
                        SerialDisposable flight;
                        flight.Set(ScheduledDisposable(
                            defaultScheduler,
                            Disposable([=]()
                            {
                                std::unique_lock<std::mutex> guard(flight_lock);
                                inflight->OnNext(false);
                            })));
                        return flight;
                    },
                    // observable factory
                    [=](SerialDisposable) -> std::shared_ptr<Observable<R>>
                    {
                        return f(t);
                    });
                })
                .observe_on(defaultScheduler)
                .publish()
                .connect_forever());
        }

        template<class R>
        std::shared_ptr<Observable<R>> RegisterAsyncFunction(std::function < R(T)> f, Scheduler::shared scheduler = nullptr)
        {
            auto asyncFunc = ToAsync<R, T>(f, scheduler);
            return RegisterAsync<R>(asyncFunc);
        }

        bool AllowsConcurrentExecution()
        {
            return allowsConcurrentExecution;
        }

        std::shared_ptr<Observable<std::exception_ptr>> ThrownExceptions()
        {
            return from(exceptions).observe_on(defaultScheduler);
        }

        std::shared_ptr<Observable<bool>> IsExecuting()
        {
            return isExecuting;
        }

        std::shared_ptr<Observable<bool>> CanExecuteObservable()
        {
            return canExecuteObservable;
        }

        Disposable Subscribe(std::shared_ptr < Observer < T >> observer)
        {
            return from(executed)
                .subscribe(
                //on next
                [=](const T& t){
                    try
                    {
                        observer->OnNext(t);
                    }
                    catch (...)
                    {
                        exceptions->OnError(std::current_exception());
                    }
                },
                //on completed
                [=](){
                    try
                    {
                        observer->OnCompleted();
                    }
                    catch (...)
                    {
                        exceptions->OnError(std::current_exception());
                    }
                },
                //on error
                [=](std::exception_ptr e){
                    try
                    {
                        observer->OnError(e);
                    }
                    catch (...)
                    {
                        exceptions->OnError(std::current_exception());
                    }
                });
        }

        void Execute(T value)
        {
            {
                std::unique_lock<std::mutex> guard(flight_lock);
                inflight->OnNext(true);
            }
            executed->OnNext(value);
            {
                std::unique_lock<std::mutex> guard(flight_lock);
                inflight->OnNext(false);
            }
        }
    };

    template<class T>
    std::shared_ptr<Observable<T>> observable(std::shared_ptr < ReactiveCommand < T >> s){ return std::static_pointer_cast < Observable < T >> (s); }

    template<class T>
    Disposable BindCommand(wuictrls::Button^ button, std::shared_ptr<ReactiveCommand<T>> command)
    {
        typedef rxrt::EventPattern<Platform::Object^, wuixaml::RoutedEventArgs^> RoutedEventPattern;

        ComposableDisposable cd;

        cd.Add(from(command->CanExecuteObservable())
            .subscribe(
            [=](bool b)
        {
            button->IsEnabled = b;
        }));

        auto click = rxrt::FromEventPattern<wuixaml::RoutedEventHandler, wuixaml::RoutedEventArgs>(
            [=](wuixaml::RoutedEventHandler^ h)
        {
            return button->Click += h;
        },
            [=](wf::EventRegistrationToken t)
        {
            button->Click -= t;
        });

        cd.Add(from(click)
            .subscribe([=](RoutedEventPattern ep)
        {
            command->Execute(ep);
        }));

        return cd;
    }

    template <class O>
    struct OperationPattern
    {
        typedef decltype(((O)nullptr)->GetDeferral()) D;

        OperationPattern(O operation) :
            operation(operation),
            deferral(operation->GetDeferral())
        {
        }

        O Operation() const {
            return operation;
        };
        D Deferral() const {
            return deferral;
        };

        void Dispose()
        {
            deferral->Complete();
        }

        operator Disposable() const
        {
            // make sure to capture state and not 'this'.
            // usage means that 'this' will usualy be destructed
            // immediately
            auto local = deferral;
            return Disposable([local]{
                local->Complete();
            });
        }

    private:
        O operation;
        D deferral;
    };

    template <class O>
    OperationPattern<O> make_operation_pattern(O o) 
    { 
        return OperationPattern<O>(std::move(o)); 
    }


    namespace detail
    {

        template<class T, class SOp, class SOb>
        auto DeferOperation(const std::shared_ptr < Observable < T >> &source, SOp sop, SOb sob, Scheduler::shared scheduler = nullptr)
            ->      decltype(sob(*(OperationPattern<decltype(sop(*(T*) nullptr))>*)nullptr, *(T*) nullptr))
        {
            typedef decltype(sob(*(OperationPattern<decltype(sop(*(T*) nullptr))>*)nullptr, *(T*) nullptr)) ResultObservable;
            typedef typename observable_item<ResultObservable>::type Result;
            if (!scheduler)
            {
                scheduler = std::static_pointer_cast<Scheduler>(winrt::CoreDispatcherScheduler::Current());
            }
            return rx::CreateObservable<Result>(
                [=](const std::shared_ptr < Observer < Result >> &observer)
                {
                    return from(source)
                        .select_many(
                        //select observable
                        [=](T t) -> ResultObservable
                        {
                            // must take the deferral early while the event is still on the stack. 
                            auto op = make_operation_pattern(sop(t));
                            typedef decltype(op) OP;
 
                            return Using<Result, OP>(
                                // resource factory
                                [=]() -> OP
                                {
                                    return op;
                                },
                                // observable factory
                                [sob, t](OP op)
                                {
                                    return sob(op, t);
                                });
                        })
                        .observe_on(scheduler)
                        .subscribe(
                        //on next
                        [=](Result r)
                        {
                            observer->OnNext(r);
                        },
                        //on completed
                        [=]()
                        {
                            observer->OnCompleted();
                        },
                        //on error
                        [=](std::exception_ptr e)
                        {
                            observer->OnError(e);
                        }
                        );
                });
        }
    }

    struct defer_operation {};
    template<class T, class SOp, class SOb>
    auto rxcpp_chain(defer_operation && , const std::shared_ptr < Observable < T >> &source, SOp sop, SOb sob, Scheduler::shared scheduler = nullptr)
        -> decltype(detail::DeferOperation(source, sop, sob, scheduler))
    {
        return      detail::DeferOperation(source, sop, sob, scheduler);
    }

} }

#endif

#endif
