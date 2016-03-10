
#include "rxcpp/rx.hpp"
using namespace rxcpp;
using namespace rxcpp::sources;
using namespace rxcpp::operators;
using namespace rxcpp::util;

#include <regex>
#include <random>
using namespace std;

int main()
{
    random_device rd;   // non-deterministic generator
    mt19937 gen(rd());
    uniform_int_distribution<> dist(4, 18);

    // for testing purposes, produce byte stream that from lines of text
    auto bytes = range(1, 10) |
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
                    [](vector<uint8_t>& v, uint8_t b){
                        v.push_back(b);
                        return move(v);
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
        filter([](string& s){
            return !s.empty();
        });

    // group strings by line
    int group = 0;
    auto linewindows = strings |
        group_by(
            [=](string& s) mutable {
                return s.back() == '\r' ? group++ : group;
            });

    // reduce the strings for a line into one string
    auto lines = linewindows |
        flat_map([](grouped_observable<int, string> w) {
            return w | sum();
        });

    // print result
    lines |
        subscribe<string>(println(cout));

    return 0;
}
