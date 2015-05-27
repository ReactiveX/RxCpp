#Developer Manual

##Overview

###What this repository provides

This repository consists of two main parts:
* `Ix`, a library modeled after the C# implementation of  [System.Linq for the .NET framework](https://msdn.microsoft.com/en-us/library/bb397926.aspx). It provides powerful database queries for any enumerable collection, which in C++ means any container that adheres to the concept of ```Iterable```
* `Rx`, a library modeled after the C# implementation of [Reactive Extensions](https://msdn.microsoft.com/en-us/data/gg577609.aspx)

The C++ implementation of the Reactive Extensions started as a direct port of the famous and successfull [C# implementation](https://github.com/Reactive-Extensions/Rx.NET). A very clear and concise introduction to C# Rx written by Lee Campbell is available at http://introtorx.com. We recommend to take at least a short glance at it because of its didactic setup and its clear and concise explanations. The major concepts are similar in the C# and the C++ versions of the libaries, but as the C++ version evolves, the difference in language features between C# and modern C++ tend to surface and therefore the architecture will differ in many aspects.

###Unit Tests

This project relies on the [Catch](https://github.com/philsquared/Catch) framework for unit tests, a lightweight, yet powerful, header-only library.

###Continious Integration

This repository is set up such that any pull request will automatically have all unit tests performed via web hooks on a build server and the results posted in the GitHub pull request issue. See e.g. the MSOTBOT entry for [this pull request](https://github.com/Reactive-Extensions/RxCpp/pull/134).

###Code Examples

[TODO: add an overview of code examples for Ix and Rx]

##The most important classes


###observable

```observable``` is the central concept of this libary. It is something you can subscribe on using an observer.

As of this writing (May 2015) this library is designed in such a way that all sequence operators are member function of the [```template<class T, class SourceOperator>
class observable```](RxCpp/blob/master/Rx/v2/src/rxcpp/rx-observable.hpp). There was some [unfinished discussion](https://twitter.com/MarkusWerle/status/599330785544577025) whether operator chaining could be done using the ```|``` operator like e.g. in the [range library](https://github.com/ericniebler/range-v3), but the current library is not designed that way. The main reason is ease of use, since member functions show up in intellisense. Once intellisense has evolved further and also displays available free functions that fit, the design might probably be changed - not now.




###observer

[TODO: describe ```observer```]


###subject
[TODO: describe ```subject```]


###schedulers

[TODO: describe ```schedulers```]

###subscriber

[TODO: describe ```subscriber```]


###composite_subscription

[TODO: describe ```composite_subscription```]





##Some comments on the scheduler system

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
