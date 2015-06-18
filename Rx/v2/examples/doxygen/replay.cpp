#include "rxcpp/rx.hpp"

#include "rxcpp/rx-test.hpp"
#include "catch.hpp"

std::string get_pid();

SCENARIO("replay sample"){
    printf("//! [replay sample]\n");
    auto values = rxcpp::observable<>::interval(std::chrono::milliseconds(50), rxcpp::observe_on_new_thread()).
        take(5).
        replay();

    // Subscribe from the beginning
    values.subscribe(
        [](long v){printf("[1] OnNext: %ld\n", v);},
        [](){printf("[1] OnCompleted\n");});

    // Start emitting
    values.connect();

    // Wait before subscribing
    rxcpp::observable<>::timer(std::chrono::milliseconds(125)).subscribe([&](long){
        values.as_blocking().subscribe(
            [](long v){printf("[2] OnNext: %ld\n", v);},
            [](){printf("[2] OnCompleted\n");});
    });
    printf("//! [replay sample]\n");
}

SCENARIO("threaded replay sample"){
    printf("//! [threaded replay sample]\n");
    printf("[thread %s] Start task\n", get_pid().c_str());
    auto coordination = rxcpp::serialize_new_thread();
    auto worker = coordination.create_coordinator().get_worker();
    auto values = rxcpp::observable<>::interval(std::chrono::milliseconds(50)).
        take(5).
        replay(coordination);

    // Subscribe from the beginning
    worker.schedule([&](const rxcpp::schedulers::schedulable&){
        values.subscribe(
            [](long v){printf("[thread %s][1] OnNext: %ld\n", get_pid().c_str(), v);},
            [](){printf("[thread %s][1] OnCompleted\n", get_pid().c_str());});
    });

    // Wait before subscribing
    worker.schedule(coordination.now() + std::chrono::milliseconds(125), [&](const rxcpp::schedulers::schedulable&){
        values.subscribe(
            [](long v){printf("[thread %s][2] OnNext: %ld\n", get_pid().c_str(), v);},
            [](){printf("[thread %s][2] OnCompleted\n", get_pid().c_str());});
    });

    // Start emitting
    worker.schedule([&](const rxcpp::schedulers::schedulable&){
        values.connect();
    });

    // Add blocking subscription to see results
    values.as_blocking().subscribe();
    printf("[thread %s] Finish task\n", get_pid().c_str());
    printf("//! [threaded replay sample]\n");
}

SCENARIO("replay count sample"){
    printf("//! [replay count sample]\n");
    auto values = rxcpp::observable<>::interval(std::chrono::milliseconds(50), rxcpp::observe_on_new_thread()).
        take(5).
        replay(2);

    // Subscribe from the beginning
    values.subscribe(
        [](long v){printf("[1] OnNext: %ld\n", v);},
        [](){printf("[1] OnCompleted\n");});

    // Start emitting
    values.connect();

    // Wait before subscribing
    rxcpp::observable<>::timer(std::chrono::milliseconds(125)).subscribe([&](long){
        values.as_blocking().subscribe(
            [](long v){printf("[2] OnNext: %ld\n", v);},
            [](){printf("[2] OnCompleted\n");});
    });
    printf("//! [replay count sample]\n");
}

SCENARIO("threaded replay count sample"){
    printf("//! [threaded replay count sample]\n");
    printf("[thread %s] Start task\n", get_pid().c_str());
    auto coordination = rxcpp::serialize_new_thread();
    auto worker = coordination.create_coordinator().get_worker();
    auto values = rxcpp::observable<>::interval(std::chrono::milliseconds(50)).
        take(5).
        replay(2, coordination);

    // Subscribe from the beginning
    worker.schedule([&](const rxcpp::schedulers::schedulable&){
        values.subscribe(
            [](long v){printf("[thread %s][1] OnNext: %ld\n", get_pid().c_str(), v);},
            [](){printf("[thread %s][1] OnCompleted\n", get_pid().c_str());});
    });

    // Wait before subscribing
    worker.schedule(coordination.now() + std::chrono::milliseconds(125), [&](const rxcpp::schedulers::schedulable&){
        values.subscribe(
            [](long v){printf("[thread %s][2] OnNext: %ld\n", get_pid().c_str(), v);},
            [](){printf("[thread %s][2] OnCompleted\n", get_pid().c_str());});
    });

    // Start emitting
    worker.schedule([&](const rxcpp::schedulers::schedulable&){
        values.connect();
    });

    // Add blocking subscription to see results
    values.as_blocking().subscribe();
    printf("[thread %s] Finish task\n", get_pid().c_str());
    printf("//! [threaded replay count sample]\n");
}

SCENARIO("replay period sample"){
    printf("//! [replay period sample]\n");
    auto values = rxcpp::observable<>::interval(std::chrono::milliseconds(50), rxcpp::observe_on_new_thread()).
        take(5).
        replay(std::chrono::milliseconds(125));

    // Subscribe from the beginning
    values.subscribe(
        [](long v){printf("[1] OnNext: %ld\n", v);},
        [](){printf("[1] OnCompleted\n");});

    // Start emitting
    values.connect();

    // Wait before subscribing
    rxcpp::observable<>::timer(std::chrono::milliseconds(175)).subscribe([&](long){
        values.as_blocking().subscribe(
            [](long v){printf("[2] OnNext: %ld\n", v);},
            [](){printf("[2] OnCompleted\n");});
    });
    printf("//! [replay period sample]\n");
}

SCENARIO("threaded replay period sample"){
    printf("//! [threaded replay period sample]\n");
    printf("[thread %s] Start task\n", get_pid().c_str());
    auto coordination = rxcpp::serialize_new_thread();
    auto worker = coordination.create_coordinator().get_worker();
    auto values = rxcpp::observable<>::interval(std::chrono::milliseconds(50)).
        take(5).
        replay(std::chrono::milliseconds(125), coordination);

    // Subscribe from the beginning
    worker.schedule([&](const rxcpp::schedulers::schedulable&){
        values.subscribe(
            [](long v){printf("[thread %s][1] OnNext: %ld\n", get_pid().c_str(), v);},
            [](){printf("[thread %s][1] OnCompleted\n", get_pid().c_str());});
    });

    // Wait before subscribing
    worker.schedule(coordination.now() + std::chrono::milliseconds(175), [&](const rxcpp::schedulers::schedulable&){
        values.subscribe(
            [](long v){printf("[thread %s][2] OnNext: %ld\n", get_pid().c_str(), v);},
            [](){printf("[thread %s][2] OnCompleted\n", get_pid().c_str());});
    });

    // Start emitting
    worker.schedule([&](const rxcpp::schedulers::schedulable&){
        values.connect();
    });

    // Add blocking subscription to see results
    values.as_blocking().subscribe();
    printf("[thread %s] Finish task\n", get_pid().c_str());
    printf("//! [threaded replay period sample]\n");
}

SCENARIO("replay count+period sample"){
    printf("//! [replay count+period sample]\n");
    auto values = rxcpp::observable<>::interval(std::chrono::milliseconds(50), rxcpp::observe_on_new_thread()).
        take(5).
        replay(2, std::chrono::milliseconds(125));

    // Subscribe from the beginning
    values.subscribe(
        [](long v){printf("[1] OnNext: %ld\n", v);},
        [](){printf("[1] OnCompleted\n");});

    // Start emitting
    values.connect();

    // Wait before subscribing
    rxcpp::observable<>::timer(std::chrono::milliseconds(175)).subscribe([&](long){
        values.as_blocking().subscribe(
            [](long v){printf("[2] OnNext: %ld\n", v);},
            [](){printf("[2] OnCompleted\n");});
    });
    printf("//! [replay count+period sample]\n");
}

SCENARIO("threaded replay count+period sample"){
    printf("//! [threaded replay count+period sample]\n");
    printf("[thread %s] Start task\n", get_pid().c_str());
    auto coordination = rxcpp::serialize_new_thread();
    auto worker = coordination.create_coordinator().get_worker();
    auto values = rxcpp::observable<>::interval(std::chrono::milliseconds(50)).
        take(5).
        replay(2, std::chrono::milliseconds(125), coordination);

    // Subscribe from the beginning
    worker.schedule([&](const rxcpp::schedulers::schedulable&){
        values.subscribe(
            [](long v){printf("[thread %s][1] OnNext: %ld\n", get_pid().c_str(), v);},
            [](){printf("[thread %s][1] OnCompleted\n", get_pid().c_str());});
    });

    // Wait before subscribing
    worker.schedule(coordination.now() + std::chrono::milliseconds(175), [&](const rxcpp::schedulers::schedulable&){
        values.subscribe(
            [](long v){printf("[thread %s][2] OnNext: %ld\n", get_pid().c_str(), v);},
            [](){printf("[thread %s][2] OnCompleted\n", get_pid().c_str());});
    });

    // Start emitting
    worker.schedule([&](const rxcpp::schedulers::schedulable&){
        values.connect();
    });

    // Add blocking subscription to see results
    values.as_blocking().subscribe();
    printf("[thread %s] Finish task\n", get_pid().c_str());
    printf("//! [threaded replay count+period sample]\n");
}
