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

    template <class TSender, class TEventArgs>
    struct EventPattern
    {
        EventPattern(TSender sender, TEventArgs eventargs) :
            sender(sender),
            eventargs(eventargs)
        {}

        TSender Sender() {
            return sender;};
        TEventArgs EventArgs() {
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

    template<typename F>
    auto FromAsyncPattern(F&& start) 
        -> std::function < std::shared_ptr < Observable< decltype(start()->GetResults()) >> ()>
    {
        return [=]()
        {
            typedef decltype(start()->GetResults()) Result;
            auto subject = CreateAsyncSubject<Result>();
            auto o = start();
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

    template<typename Operation, typename Result, typename... T>
    std::function < std::shared_ptr < Observable<Result >> (const T&...)> FromAsyncPattern(std::function<Operation^(const T&...)> start)
    {
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
            return std::make_shared<CoreDispatcherScheduler>(window->Dispatcher);
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

} }

#endif

#endif
