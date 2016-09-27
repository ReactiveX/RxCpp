#include <exception>
#if (__GLIBCXX__ / 10000) == 2014 || (__GLIBCXX__ / 10000) == 2015
namespace std {
inline bool uncaught_exception() noexcept(true) {
    return current_exception() != nullptr;
}
}
#endif

#include "rxcpp/rx-lite.hpp"
namespace rx=rxcpp;
namespace rxu=rxcpp::util;
namespace rxs=rxcpp::sources;
namespace rxo=rxcpp::operators;
namespace rxsub=rxcpp::subjects;
namespace rxsc=rxcpp::schedulers;
namespace rxn=rx::notifications;

#include "rxcpp/rx-test.hpp"
namespace rxt = rxcpp::test;

#include "catch.hpp"
