
#include <rxcpp/rx-lite.hpp>
#include <rxcpp/operators/rx-take.hpp>

#include <rxcpp/rx-coroutine.hpp>

using namespace rxcpp;
using namespace rxcpp::sources;
using namespace rxcpp::operators;
using namespace rxcpp::util;

using namespace std;
using namespace std::chrono;

future<void> intervals(){

    {
        printf("early exit from interval on thread\n");
        for co_await (auto c : interval(seconds(1), observe_on_event_loop())) {
            printf("%d\n", c);
            break;
        }
    }

    {
        printf("interval on thread\n");
        for co_await (auto c : interval(seconds(1), observe_on_event_loop()) | take(3)) {
            printf("%d\n", c);
        }
    }

    {
        printf("current thread\n");
        int last = 0;
        for co_await (auto c : range(1, 100000)) {
            last = c;
        }
        printf("reached %d\n", last);
    }

    try {
        printf("error in observable\n");
        for co_await (auto c : error<long>(runtime_error("stopped by error"))) {
            printf("%d\n", c);
        }
        printf("not reachable\n");
        terminate();
    }
    catch(const exception& e) {
        printf("%s\n", e.what());
    }
}

int main()
{
    intervals().get();
    return 0;
}
