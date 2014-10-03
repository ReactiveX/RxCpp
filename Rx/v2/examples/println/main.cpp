#include "rxcpp/rx.hpp"
// create alias' to simplify code
// these are owned by the user so that
// conflicts can be managed by the user.
namespace rx=rxcpp;
namespace rxu=rxcpp::util;

#include<string>

// At this time, RxCpp will fail to compile if the contents
// of the std namespace are merged into the global namespace
// DO NOT USE: 'using namespace std;'

#if 0
//
// println will insert values into the specified stream
//
template<class OStream>
struct println_function
{
    OStream& os;
    println_function(OStream& os) : os(os) {}

    template<class... TN>
    void operator()(const TN&... tn) const {
        bool inserts[] = {(os << tn, true)...};
        os << std::endl;
    }

    template<class... TN>
    void operator()(const std::tuple<TN...>& tpl) const {
        apply(tpl, *this);
    }
};
template<class OStream>
auto println(OStream& os)
    ->      println_function<OStream> {
    return  println_function<OStream>(os);
}
#endif

#ifdef UNICODE
int wmain()
#else
int main()
#endif
{
    auto get_names = [](){return rx::observable<>::from<std::string>(
        "Matthew",
        "Aaron"
    );};

    std::cout << "===== println stream of std::string =====" << std::endl;
    auto hello_str = [&](){return get_names().map([](std::string n){
        return "Hello, " + n + "!";
    }).as_dynamic();};

    hello_str().subscribe(rxu::println(std::cout));

    std::cout << "===== println stream of std::tuple =====" << std::endl;
    auto hello_tpl = [&](){return get_names().map([](std::string n){
        return std::make_tuple("Hello, ", n, "! (", n.size(), ")");
    }).as_dynamic();};

    hello_tpl().subscribe(rxu::println(std::cout));

    hello_tpl().subscribe(rxu::print_followed_by(std::cout, " and "), rxu::endline(std::cout));
    return 0;
}
