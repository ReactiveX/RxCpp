// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

// testbench.cpp : Defines the entry point for the console application.
//

#include "cpprx/rx.hpp"
#include "cpplinq/linq.hpp"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <exception>
#include <regex>

using namespace std;

bool IsPrime(int x);

void PrintPrimes(int n)
{
    std::cout << "first " << n << " primes squared" << endl;
    auto values = rxcpp::Range(2); // infinite (until overflow) stream of integers
    auto s1 = rxcpp::from(values)
        .where(IsPrime)
        .select([](int x) { return std::make_pair(x,  x*x); })
        .take(n)
        .subscribe(
            [](pair<int, int> p) {
                cout << p.first << " =square=> " << p.second << endl;
            });
}

std::shared_ptr<rxcpp::Observable<string>> Data(
    string filename,
    rxcpp::Scheduler::shared scheduler = std::make_shared<rxcpp::CurrentThreadScheduler>()
);
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

    cout << "data >input ^parse and <push_back:" << endl;
    auto shared_data_parsed = std::make_shared<std::vector<item>>();
    {
        std::exception_ptr error;
        std::condition_variable cv;

        std::mutex coutLock;
        int inID = 0;
        int parseID = 0;
        int outID = 0;

        auto dataLines = Data(
            "data.txt"
//            , std::make_shared<rxcpp::EventLoopScheduler>()
        );
        auto input = std::make_shared<rxcpp::NewThreadScheduler>();
        auto parsing = std::make_shared<rxcpp::EventLoopScheduler>();
        auto output = std::make_shared<rxcpp::EventLoopScheduler>();
        auto exp = rxcpp::from(dataLines)
            .subscribe_on(input)
            .select([&coutLock, &inID](const string& line) { 
                {
                    std::unique_lock<std::mutex> guard(coutLock);
                    if (inID++ == 0) std::cout << std::this_thread::get_id(); 
                    std::cout << '>' << std::flush;
                }
                return line; })
            .observe_on(parsing)
            .select([&coutLock, &parseID](const string& line) { 
                {
                    std::unique_lock<std::mutex> guard(coutLock);
                    if (parseID++ == 0) std::cout << std::this_thread::get_id(); 
                    std::cout << '^' << std::flush;
                }
                return item(line); })
            .observe_on(output)
//            .delay(std::chrono::milliseconds(10), output)
            .subscribe(
               [=, &coutLock, &outID](const item& i){
                    {
                        std::unique_lock<std::mutex> guard(coutLock);
                        if (outID++ == 0) std::cout << std::this_thread::get_id(); 
                        std::cout << '<' << std::flush;
                    }
                    shared_data_parsed->push_back(i); },
               [&cv](){
                   cv.notify_one();},
               [&cv, &error](const std::exception_ptr& e){
                   error = e; cv.notify_one();});

        std::mutex doneLock;
        std::unique_lock<decltype(doneLock)> guard(doneLock);
        cv.wait(guard);
        if (error != std::exception_ptr()) {std::rethrow_exception(error);}
        cout << endl << "data loaded" << endl;
    }

    auto data_parsed = shared_data_parsed.get();
    auto data = 
        from(*data_parsed)        
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

int main(int argc, char* argv[])
{
    PrintPrimes(20);

    try {
        run();
    } catch (exception& e) {
        cerr << "exception: " << e.what() << endl;
    }
}

bool IsPrime(int x)
{
    if (x < 2) return false;
    for (int i = 2; i <= x/2; ++i)
    {
        if (x % i == 0) 
            return false;
    }
    return true;
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

std::shared_ptr<rxcpp::Observable<string>> Data(
    string filename,
    rxcpp::Scheduler::shared scheduler
)
{
    return rxcpp::CreateObservable<string>(
        [=](std::shared_ptr<rxcpp::Observer<string>> observer) 
        -> rxcpp::Disposable
        {
            struct State 
            {
                State(string filename) 
                    : cancel(false), data(filename) {
                        if (data.fail()) {
                            throw logic_error("could not find file");
                        }
                    }
                bool cancel;
                ifstream data;
            };
            auto state = std::make_shared<State>(std::move(filename));

            rxcpp::ComposableDisposable cd;

            cd.Add(rxcpp::Disposable([=]{
                state->cancel = true;
            }));

            cd.Add(scheduler->Schedule(
                rxcpp::fix0([=](rxcpp::Scheduler::shared s, std::function<rxcpp::Disposable(rxcpp::Scheduler::shared)> self) -> rxcpp::Disposable
                {
                    try {
                        if (state->cancel)
                            return rxcpp::Disposable::Empty();

                        string line;
                        if (!!getline(state->data, line))
                        {
                            observer->OnNext(std::move(line));
                            return s->Schedule(std::move(self));
                        }
                        else
                        {
                            observer->OnCompleted();
                        }
                    } catch (...) {
                        observer->OnError(std::current_exception());
                    }     
                    return rxcpp::Disposable::Empty();           
                })
            ));

            return cd;
        }
    );
}

