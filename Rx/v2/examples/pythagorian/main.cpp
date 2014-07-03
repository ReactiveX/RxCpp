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
int wmain(int argc, wchar_t** argv)
#else
int main(int argc, char** argv)
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
                            [](int x, std::tuple<int,int,int> triplet){return triplet;})
                        // forget type to workaround lambda deduction bug on msvc 2013
                        .as_dynamic();},
                [](int z, std::tuple<int,int,int> triplet){return triplet;});

    int ct = 0;

    triples
        .take(100)
        .subscribe(rxu::apply_to([&ct](int x,int y,int z){
            ++ct;
        }));

    std::cout << "concat_map pythagorian range : " << c << " filtered to, " << ct << " triplets" << std::endl;

    return 0;
}
