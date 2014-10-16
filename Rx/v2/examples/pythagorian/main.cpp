
#include "rxcpp/rx-trace.hpp"

struct trace_calls : rxcpp::trace_noop
{
    trace_calls()
        : onnexts(0)
        , onerrors(0)
        , oncompleteds(0)
        , subscribes(0)
        , lifts(0)
        , unsubscribes(0)
        , adds(0)
        , removes(0)
        , actions(0)
        , recurses(0)
        , schedules(0)
        , schedulewhens(0)
    {}

    template<class Worker>
    inline void schedule_return(const Worker&) {
        schedules++;
    }

    template<class Worker>
    inline void schedule_when_return(const Worker&) {
        schedulewhens++;
    }

    template<class Schedulable>
    inline void action_return(const Schedulable&) {
        actions++;
    }

    template<class Schedulable>
    inline void action_recurse(const Schedulable&) {
        recurses++;
    }

    template<class Observable>
    inline void subscribe_return(const Observable&) {
        subscribes++;
    }

    template<class OperatorSource, class OperatorChain>
    inline void lift_return(const OperatorSource&, const OperatorChain&) {
        lifts++;
    }

    template<class SubscriptionState>
    inline void unsubscribe_return(const SubscriptionState&) {
        unsubscribes++;
    }

    template<class SubscriptionState>
    inline void subscription_add_return(const SubscriptionState&) {
        adds++;
    }

    template<class SubscriptionState>
    inline void subscription_remove_return(const SubscriptionState&) {
        removes++;
    }

    template<class Observer>
    inline void on_next_return(const Observer&) {
        onnexts++;
    }

    template<class Observer>
    inline void on_error_return(const Observer&) {
        onerrors++;
    }

    template<class Observer>
    inline void on_completed_return(const Observer&) {
        oncompleteds++;
    }

    int onnexts;
    int onerrors;
    int oncompleteds;
    int subscribes;
    int lifts;
    int unsubscribes;
    int adds;
    int removes;
    int actions;
    int recurses;
    int schedules;
    int schedulewhens;
};

auto rxcpp_trace_activity(rxcpp::trace_tag) -> trace_calls;

#include "rxcpp/rx.hpp"
// create alias' to simplify code
// these are owned by the user so that
// conflicts can be managed by the user.
namespace rx=rxcpp;
namespace rxu=rxcpp::util;

// At this time, RxCpp will fail to compile if the contents
// of the std namespace are merged into the global namespace
// DO NOT USE: 'using namespace std;'

#ifdef UNICODE
int wmain()
#else
int main()
#endif
{
    int c = 0;

    auto triples =
        rx::observable<>::range(1)
            .concat_map(
                [&c](int z){
                    return rx::observable<>::range(1, z)
                        .concat_map(
                            [=, &c](int x){
                                return rx::observable<>::range(x, z)
                                    .filter([=, &c](int y){++c; return x*x + y*y == z*z;})
                                    .map([=](int y){return std::make_tuple(x, y, z);})
                                    // forget type to workaround lambda deduction bug on msvc 2013
                                    .as_dynamic();},
                            [](int /*x*/, std::tuple<int,int,int> triplet){return triplet;})
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();},
                [](int /*z*/, std::tuple<int,int,int> triplet){return triplet;});

    int ct = 0;

    triples
        .take(100)
        .subscribe(rxu::apply_to([&ct](int /*x*/,int /*y*/,int /*z*/){
            ++ct;
        }));

    std::cout << "concat_map pythagorian range : " << c << " filtered to, " << ct << " triplets." << std::endl;
    std::cout << "onnexts: " << rxcpp::trace_activity().onnexts << ", onerrors: " << rxcpp::trace_activity().onerrors << ", oncompleteds: " << rxcpp::trace_activity().oncompleteds << std::endl;
    std::cout << "subscribes: " << rxcpp::trace_activity().subscribes << ", lifts: " << rxcpp::trace_activity().lifts << std::endl;
    std::cout << "unsubscribes: " << rxcpp::trace_activity().unsubscribes << ", adds: " << rxcpp::trace_activity().adds << ", removes: " << rxcpp::trace_activity().removes << std::endl;
    std::cout << "schedules: " << rxcpp::trace_activity().schedules << ", schedulewhens: " << rxcpp::trace_activity().schedulewhens << std::endl;
    std::cout << "actions: " << rxcpp::trace_activity().actions << ", recurses: " << rxcpp::trace_activity().recurses << std::endl;

    return 0;
}
