
#include "rxcpp/rx.hpp"
// create alias' to simplify code
// these are owned by the user so that
// conflicts can be managed by the user.
namespace rx=rxcpp;
namespace rxsub=rxcpp::subjects;
namespace rxu=rxcpp::util;

#include <cctype>
#include <clocale>

// At this time, RxCpp will fail to compile if the contents
// of the std namespace are merged into the global namespace
// DO NOT USE: 'using namespace std;'

int main()
{
    auto keys = rx::observable<>::create<int>(
        [](rx::subscriber<int> dest){
            for (;;) {
                int key = std::cin.get();
                dest.on_next(key);
            }
        }).
        publish();

    auto a = keys.
        filter([](int key){return std::tolower(key) == 'a';});

    auto g = keys.
        filter([](int key){return std::tolower(key) == 'g';});

    a.merge(g).
        subscribe([](int key){
            std::cout << key << std::endl;
        });

    // run the loop in create
    keys.connect();

    return 0;
}
