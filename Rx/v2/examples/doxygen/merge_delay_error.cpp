#include "rxcpp/rx.hpp"
namespace rxu=rxcpp::util;

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

SCENARIO("merge_delay_error sample"){
	printf("//! [merge_delay_error sample]\n");
	auto o1 = rxcpp::observable<>::timer(std::chrono::milliseconds(15)).map([](int) {return 1;});
	auto o2 = rxcpp::observable<>::error<int>(std::runtime_error("Error from source\n"));
	auto o3 = rxcpp::observable<>::timer(std::chrono::milliseconds(5)).map([](int) {return 3;});
	auto values = o1.merge_delay_error(o2, o3);
	values.
		subscribe(
			[](int v){printf("OnNext: %d\n", v);},
			[](std::exception_ptr eptr) { printf("OnError %s\n", rxu::what(eptr).c_str()); },
			[](){printf("OnCompleted\n");});
	printf("//! [merge_delay_error sample]\n");
}

SCENARIO("implicit merge_delay_error sample"){
	printf("//! [implicit merge_delay_error sample]\n");
	auto o1 = rxcpp::observable<>::timer(std::chrono::milliseconds(15)).map([](int) {return 1;});
	auto o2 = rxcpp::observable<>::error<int>(std::runtime_error("Error from source\n"));
	auto o3 = rxcpp::observable<>::timer(std::chrono::milliseconds(5)).map([](int) {return 3;});
	auto base = rxcpp::observable<>::from(o1.as_dynamic(), o2, o3);
	auto values = base.merge();
	values.
		subscribe(
			[](int v){printf("OnNext: %d\n", v);},
			[](std::exception_ptr eptr) { printf("OnError %s\n", rxu::what(eptr).c_str()); },
			[](){printf("OnCompleted\n");});
	printf("//! [implicit merge_delay_error sample]\n");
}

std::string get_pid();

SCENARIO("threaded merge_delay_error sample"){
	printf("//! [threaded merge_delay_error sample]\n");
	printf("[thread %s] Start task\n", get_pid().c_str());
	auto o1 = rxcpp::observable<>::timer(std::chrono::milliseconds(10)).map([](int) {
		printf("[thread %s] Timer1 fired\n", get_pid().c_str());
		return 1;
	});
	auto o2 = rxcpp::observable<>::timer(std::chrono::milliseconds(20)).flat_map([](int) {
		std::stringstream ss;
		ss << "[thread " << get_pid().c_str() << "] Timer2 failed\n";
		printf("%s\n", ss.str().c_str());
		ss.str(std::string());
		ss << "(Error from thread: " << get_pid().c_str() << ")\n";
		return rxcpp::observable<>::error<int>(std::runtime_error(ss.str()));
	});
	auto o3 = rxcpp::observable<>::timer(std::chrono::milliseconds(30)).map([](int) {
		printf("[thread %s] Timer3 fired\n", get_pid().c_str());
		return 3;
	});
	auto values = o1.merge(rxcpp::observe_on_new_thread(), o2, o3);
	values.
		as_blocking().
		subscribe(
			[](int v){printf("[thread %s] OnNext: %d\n", get_pid().c_str(), v);},
			[](std::exception_ptr eptr) { printf("[thread %s] OnError %s\n", get_pid().c_str(), rxu::what(eptr).c_str()); },
			[](){printf("[thread %s] OnCompleted\n", get_pid().c_str());});
	printf("[thread %s] Finish task\n", get_pid().c_str());
	printf("//! [threaded merge_delay_error sample]\n");
}

SCENARIO("threaded implicit merge_delay_error sample"){
	printf("//! [threaded implicit merge_delay_error sample]\n");
	printf("[thread %s] Start task\n", get_pid().c_str());
	auto o1 = rxcpp::observable<>::timer(std::chrono::milliseconds(10)).map([](int) {
		printf("[thread %s] Timer1 fired\n", get_pid().c_str());
		return 1;
	});
	auto o2 = rxcpp::observable<>::timer(std::chrono::milliseconds(20)).flat_map([](int) {
		std::stringstream ss;
		ss << "[thread " << get_pid().c_str() << "] Timer2 failed\n";
		printf("%s\n", ss.str().c_str());
		ss.str(std::string());
		ss << "(Error from thread: " << get_pid().c_str() << ")\n";
		return rxcpp::observable<>::error<int>(std::runtime_error(ss.str()));
	});
	auto o3 = rxcpp::observable<>::timer(std::chrono::milliseconds(30)).map([](int) {
		printf("[thread %s] Timer3 fired\n", get_pid().c_str());
		return 3;
	});
	auto base = rxcpp::observable<>::from(o1.as_dynamic(), o2, o3);
	auto values = base.merge(rxcpp::observe_on_new_thread());
	values.
		as_blocking().
		subscribe(
			[](int v){printf("[thread %s] OnNext: %d\n", get_pid().c_str(), v);},
			[](std::exception_ptr eptr) { printf("[thread %s] OnError %s\n", get_pid().c_str(), rxu::what(eptr).c_str()); },
			[](){printf("[thread %s] OnCompleted\n", get_pid().c_str());});
	printf("[thread %s] Finish task\n", get_pid().c_str());
	printf("//! [threaded implicit merge_delay_error sample]\n");
}
