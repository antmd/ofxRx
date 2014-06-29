// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_BEHAVIOR_HPP)
#define RXCPP_RX_BEHAVIOR_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace subjects {

namespace detail {

template<class T>
class behavior_observer : public detail::multicast_observer<T>
{
    typedef behavior_observer<T> this_type;
    typedef detail::multicast_observer<T> base_type;

    class behavior_observer_state : public std::enable_shared_from_this<behavior_observer_state>
    {
        mutable std::mutex lock;
        mutable T value;

    public:
        behavior_observer_state(T first)
            : value(first)
        {
        }

        void reset(T v) const {
            std::unique_lock<std::mutex> guard(lock);
            value = std::move(v);
        }
        T get() const {
            std::unique_lock<std::mutex> guard(lock);
            return value;
        }
    };

    std::shared_ptr<behavior_observer_state> state;

public:
    behavior_observer(T f, composite_subscription l)
        : base_type(l)
        , state(std::make_shared<behavior_observer_state>(std::move(f)))
    {
    }

    T get_value() const {
        return state->get();
    }

    template<class V>
    void on_next(V v) const {
        state->reset(v);
        base_type::on_next(std::move(v));
    }
};

}

template<class T>
class behavior
{
    composite_subscription lifetime;
    detail::behavior_observer<T> s;

public:
    explicit behavior(T f, composite_subscription cs = composite_subscription())
        : lifetime(std::move(cs))
        , s(std::move(f), lifetime)
    {
    }

    bool has_observers() const {
        return s.has_observers();
    }

    T get_value() const {
        return s.get_value();
    }

    subscriber<T> get_subscriber() const {
        return make_subscriber<T>(lifetime, make_observer_dynamic<T>(observer<T, detail::behavior_observer<T>>(s)));
    }

    observable<T> get_observable() const {
        return make_observable_dynamic<T>([this](subscriber<T> o){
            if (lifetime.is_subscribed()) {
                o.on_next(get_value());
            }
            this->s.add(std::move(o));
        });
    }
};

}

}

#endif
