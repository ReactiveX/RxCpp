
#include <rxcpp/rx-lite.hpp>
#include <rxcpp/operators/rx-reduce.hpp>
#include <rxcpp/operators/rx-filter.hpp>
#include <rxcpp/operators/rx-map.hpp>
#include <rxcpp/operators/rx-tap.hpp>
#include <rxcpp/operators/rx-concat_map.hpp>
#include <rxcpp/operators/rx-flat_map.hpp>
#include <rxcpp/operators/rx-concat.hpp>
#include <rxcpp/operators/rx-merge.hpp>
#include <rxcpp/operators/rx-repeat.hpp>
#include <rxcpp/operators/rx-publish.hpp>
#include <rxcpp/operators/rx-ref_count.hpp>
#include <rxcpp/operators/rx-window.hpp>
#include <rxcpp/operators/rx-window_toggle.hpp>
#include <rxcpp/operators/rx-start_with.hpp>
namespace Rx {
using namespace rxcpp;
using namespace rxcpp::sources;
using namespace rxcpp::operators;
using namespace rxcpp::util;
}
using namespace Rx;

#include <regex>
#include <random>
using namespace std;
using namespace std::chrono;

int main()
{
    random_device rd;   // non-deterministic generator
    mt19937 gen(rd());
    uniform_int_distribution<> dist(4, 18);

    // for testing purposes, produce byte stream that from lines of text
    auto bytes = range(0, 10) |
        flat_map([&](int i){
            auto body = from((uint8_t)('A' + i)) |
                repeat(dist(gen)) |
                as_dynamic();
            auto delim = from((uint8_t)'\r');
            return from(body, delim) | concat();
        }) |
        window(17) |
        flat_map([](observable<uint8_t> w){
            return w |
                reduce(
                    vector<uint8_t>(),
                    [](vector<uint8_t> v, uint8_t b){
                        v.push_back(b);
                        return v;
                    }) |
                as_dynamic();
        }) |
        tap([](vector<uint8_t>& v){
            // print input packet of bytes
            copy(v.begin(), v.end(), ostream_iterator<long>(cout, " "));
            cout << endl;
        });

    //
    // recover lines of text from byte stream
    //
    
    auto removespaces = [](string s){
        s.erase(remove_if(s.begin(), s.end(), ::isspace), s.end());
        return s;
    };

    // create strings split on \r
    auto strings = bytes |
        concat_map([](vector<uint8_t> v){
            string s(v.begin(), v.end());
            regex delim(R"/(\r)/");
            cregex_token_iterator cursor(&s[0], &s[0] + s.size(), delim, {-1, 0});
            cregex_token_iterator end;
            vector<string> splits(cursor, end);
            return iterate(move(splits));
        }) |
        filter([](const string& s){
            return !s.empty();
        }) |
        publish() |
        ref_count();

    // filter to last string in each line
    auto closes = strings |
        filter(
            [](const string& s){
                return s.back() == '\r';
            }) |
        Rx::map([](const string&){return 0;});

    // group strings by line
    auto linewindows = strings |
        window_toggle(closes | start_with(0), [=](int){return closes;});

    // reduce the strings for a line into one string
    auto lines = linewindows |
        flat_map([&](observable<string> w) {
            return w | start_with<string>("") | sum() | Rx::map(removespaces);
        });

    // print result
    lines |
        subscribe<string>(println(cout));

    return 0;
}
