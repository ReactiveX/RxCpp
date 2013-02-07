// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

// SampleCppLinq.cpp : Defines the entry point for the console application.
//

#include <cpplinq/linq.hpp>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <exception>
#include <regex>
#include <cmath>
#include <algorithm>
#include <numeric>

using namespace std;

vector<string> load_data();
string extract_value(const string& input, const string& key);


void run()
{
    using namespace cpplinq;

    struct item {
        string args;
        int    concurrency;
        double time;

        item(const string& input) {
            args =              extract_value(input, "args");
            concurrency = atoi( extract_value(input, "concurrency").c_str() );
            time =        atof( extract_value(input, "time").c_str() );
        }
    };

    auto data_unparsed = load_data();
    auto data_parsed = 
        from(data_unparsed)
        .select([](const string& line) { return item(line); })
        .to_vector();
    
    cout << "data loaded" << endl;

    auto data = 
        from(data_parsed)        
        .groupby([](const item& i) { return i.args; });
    
    for (auto giter = data.begin(), end = data.end(); giter != end; ++giter)
    {
        const auto& g = *giter;

        cout << "arguments: " << g.key << endl;

        cout << "concurrency, mean, |, raw_data," << endl;
        auto seq =
            from(g)
            .groupby([](const item& i) { return i.concurrency; });

        for (auto giter = seq.begin(), end = seq.end(); giter != end; ++giter)
        {
            const auto& g = *giter;

            cout << g.key << ", ";
            
            auto times = from(g).select([](const item& i) { return i.time; });
            
            auto n = from(g).count();
            auto sum = std::accumulate(times.begin(), times.end(), 0.0);

            cout << (sum / n) << ", |";

            for (auto timeIter = times.begin(), end = times.end();
                timeIter != end;
                ++timeIter)
            {
                cout << ", " << *timeIter;
            }
            cout << endl;
        }
    }
}


int main()
{
    try {
        run();
    } catch (exception& e) {
        cerr << "exception: " << e.what() << endl;
    }
}

vector<string> load_data()
{
	ifstream datafile("data.txt");
    vector<string> v;
    string line;

    if (datafile.fail())
        throw logic_error("could not find file");

    while(getline(datafile, line))
        v.push_back(line);

    return v;
}

regex key_value_pair("'([^\']*)'\\s*[:,]\\s*(\\d+(?:\\.\\d+)?|'[^']*')");

string extract_value(const string& input, const string& key)
{
    const std::sregex_iterator end;
    for (std::sregex_iterator i(input.cbegin(), input.cend(), key_value_pair);
        i != end;
        ++i)
    {
        if ((*i)[1] == key)
        {
            return (*i)[2];
        }
    }
    throw std::range_error("search key not found");
}
