
#include "rxcpp/rx.hpp"
// create alias' to simplify code
// these are owned by the user so that
// conflicts can be managed by the user.
namespace rx=rxcpp;
namespace rxsub=rxcpp::subjects;
namespace rxu=rxcpp::util;

// At this time, RxCpp will fail to compile if the contents
// of the std namespace are merged into the global namespace
// DO NOT USE: 'using namespace std;'

int main()
{
    // works
    {
        auto published_observable =
            rx::observable<>::range(1)
            .filter([](int i)
            {
                std::cout << i << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                return true;
            })
            .subscribe_on(rx::observe_on_new_thread())
            .publish();

        auto subscription = published_observable.connect();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        subscription.unsubscribe();
        std::cout << "unsubscribed" << std::endl << std::endl;
    }

    // idiomatic (prefer operators)
    {
        auto published_observable =
            rx::observable<>::interval(std::chrono::milliseconds(300))
            .subscribe_on(rx::observe_on_new_thread())
            .publish();

        published_observable.
            ref_count().
            take_until(rx::observable<>::timer(std::chrono::seconds(1))).
            finally([](){
                std::cout << "unsubscribed" << std::endl << std::endl;
            }).
            subscribe([](int i){
                std::cout << i << std::endl;
            });
    }

    return 0;
}
