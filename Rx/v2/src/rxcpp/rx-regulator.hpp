// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_REGULATOR_HPP)
#define RXCPP_RX_REGULATOR_HPP

#include "rx-includes.hpp"

namespace rxcpp {

struct tag_resumption {};
struct resumption_base
{
    typedef tag_resumption resumption_tag;
};
template<class T>
class is_resumption
{
    template<class C>
    static typename C::resumption_tag* check(int);
    template<class C>
    static void check(...);
public:
    static const bool value = std::is_convertible<decltype(check<T>(0)), tag_resumption*>::value;
};

namespace detail {

struct regulator_state_type
	: public std::enable_shared_from_this<regulator_state>
{
	regulator_state_type()
		: isresumed(true)
	{
	}
	rxsc::schedulable resumewith;
	bool isresumed;
};
typedef std::shared_ptr<regulator_state_type> regulator_state;

}

//
// passed to subscribe to control the source
// the source must call is_resumed before each onnext
// the source must call resume_with when is_resumed returns false
// the schedulable passed to resume_with must implement a resumption policy;
// forward(undoes drop), unblock(undoes block), subscribe(undoes unsubscribe),
// drain(undoes buffer)
//
class resumption : public resumption_base
{
	detail::regulator_state state;
public:
	resumption(){}
	explicit resumption(detail::regulator_state st)
		: state(st)
	{
	}
	inline bool is_resumed() const {
		return state ? state->isresumed : true;
	}
	inline void resume_with(rxsc::schedulable rw) {
		// invalid to call when state is null.
		state->resumewith = rw;
	}
};

//
// owned by observer. the
// observer calls pause to stop the source. The
// observer calls resume when ready for the
// source to send more data.
//
class regulator
{
	detail::regulator_state state;
public:
	regulator()
		: state(new detail::regulator_state_type())
	{
	}
	inline void pause() {
		state->isresumed = false;
	}
	inline void resume() {
		state->isresumed = true;
        auto resumewith = std::move(state->resumewith);
        state->resumewith = nullptr;
        resumewith.schedule();
	}
	inline resumption get_resumption() {
		return resumption(state);
	}
};

}

#endif
