#pragma once

#include <memory>

class copy_verifier
{
public:
    copy_verifier()
        : _state(std::make_shared<state>()) {}

    copy_verifier(const copy_verifier& other)
        : _state{other._state}
    {
        ++_state->copy_count;
    }

    copy_verifier(copy_verifier&& other) noexcept
        : _state{other._state}
    {
        ++_state->move_count;
    }

    copy_verifier& operator=(const copy_verifier& other)
    {
        if (this == &other)
            return *this;
        _state = other._state;
        ++_state->copy_count;
        return *this;
    }

    copy_verifier& operator=(copy_verifier&& other) noexcept
    {
        if (this == &other)
            return *this;
        _state = other._state;
        ++_state->move_count;
        return *this;
    }

    int get_copy_count() const { return _state->copy_count; }
    int get_move_count() const { return _state->move_count; }

    rxcpp::observable<copy_verifier> get_observable(size_t count = 1)
    {
        return rxcpp::observable<>::create<copy_verifier>([this, count](rxcpp::subscriber<copy_verifier> sub)
        {
            for (size_t i =0; i< count; ++i)
                sub.on_next(*this);
            sub.on_completed();
        });
    }

    rxcpp::observable<copy_verifier> get_observable_for_move(size_t count = 1)
    {
        return rxcpp::observable<>::create<copy_verifier>([this, count](rxcpp::subscriber<copy_verifier> sub)
        {
            for (size_t i =0; i< count; ++i)
                sub.on_next(std::move(*this));
            sub.on_completed();
        });
    }

    bool operator==(const copy_verifier&) const { return false; }
    bool operator!=(const copy_verifier&) const { return true; }
private:
    struct state
    {
        int copy_count = 0;
        int move_count = 0;
    };

    std::shared_ptr<state> _state;
};
