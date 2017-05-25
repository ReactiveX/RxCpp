# Developer Manual

## Some comments on the scheduler system

The scheduler in rxcpp v2 is based on the scheduler and worker constructs that *RxJava* uses (Eric Meijer was involved) The docs for *RxJava* will have an explanation for ```scheduler``` and ```worker```. RxCpp adds ```schedulable```, ```coordination``` and ```coordinator```.

```scheduler``` owns a timeline that is exposed by the ```now()``` method. ```scheduler``` is also a factory for workers in that timeline. Since a scheduler owns a timeline it is possible to build schedulers that time-travel. The virtual-scheduler is a base for the test-scheduler that uses this to make multi-second tests complete in ms.

```worker``` owns a queue of pending ```schedulable```s for the timeline and has a lifetime. When the time for an ```schedulable``` is reached the ```schedulable``` is run. The queue maintains insertion order so that when N ```schedulable```s have the same target time they are run in the order that they were inserted into the queue. The ```worker``` guarantees that each ```schedulable``` completes before the next ```schedulable``` is started. when the ```worker```'s lifetime is unsubscribed all pending ```schedulable```s are discarded.

```schedulable``` owns a function and has a ```worker``` and a ```lifetime```. When the ```schedulable```'s ```lifetime``` is unsubscribed the ```schedulable``` function will not be called. The ```schedulable``` is passed to the function and allows the function to reschedule itself or schedule something else on the same worker.

The new concepts are ```coordination``` and ```coordinator```. I added these to simplify operator implementations and to introduce pay-for-use in operator implementations. Specifically, in Rx.NET and RxJava, the operators use atomic operations and synchronization primitives to coordinate messages from multiple streams even when all the streams are on the same thread (like UI events). The ```identity_...``` coordinations in RxCpp are used by default and have no overhead. The ```syncronize_...``` and ```observe_on_...``` coordinations use mutex and queue-onto-a-worker respectively, to interleave multiple streams safely.

```coordination``` is a factory for ```coordinator```s and has a scheduler.

```coordinator``` has a ```worker```, and is a factory for coordinated observables, subscribers and schedulable functions.

All the operators that take multiple streams or deal in time (even ```subscribe_on``` and ```observe_on```) take a coordination parameter, not scheduler.

Here are some supplied functions that will produce a coordination using a particular scheduler.

* ```identity_immediate()```
* ```identity_current_thread()```
* ```identity_same_worker(worker w)```
* ```serialize_event_loop()```
* ```serialize_new_thread()```
* ```serialize_same_worker(worker w)```
* ```observe_on_event_loop()```
* ```observe_on_new_thread()```

There is no thread-pool scheduler yet. A thread-pool scheduler requires taking a dependency on a thread-pool implementation since I do not wish to write a thread-pool. My plan is to make a scheduler for the windows thread-pool and the apple thread-pool and the boost asio executor pool.. One question to answer is whether these platform specific constructs should live in the rxcpp repo or have platform specific repos.
