// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_RX_SUBSCRIBER_HPP)
#define RXCPP_RX_SUBSCRIBER_HPP

#include "rx-includes.hpp"

namespace rxcpp {

template<class T>
struct subscriber_base : public observer_base<T>, public subscription_base
{
    typedef tag_subscriber subscriber_tag;
};

template<class T, class Observer = observer<T>>
class subscriber : public subscriber_base<T>
{
    typedef subscriber<T, Observer> this_type;
    typedef typename std::decay<Observer>::type observer_type;

    composite_subscription lifetime;
    observer_type destination;

    struct detacher
    {
        ~detacher()
        {
            if (that) {
                that->unsubscribe();
            }
        }
        detacher(const this_type* that)
            : that(that)
        {
        }
        const this_type* that;
    };

    subscriber();
public:
    typedef typename composite_subscription::weak_subscription weak_subscription;

    subscriber(const this_type& o)
        : lifetime(o.lifetime)
        , destination(o.destination)
    {
    }
    subscriber(this_type&& o)
        : lifetime(std::move(o.lifetime))
        , destination(std::move(o.destination))
    {
    }

    template<class U>
    subscriber(composite_subscription cs, U&& o)
        : lifetime(std::move(cs))
        , destination(std::forward<U>(o))
    {
    }

    this_type& operator=(this_type o) {
        lifetime = std::move(o.lifetime);
        destination = std::move(o.destination);
        return *this;
    }

    const observer_type& get_observer() const {
        return destination;
    }
    observer_type& get_observer() {
        return destination;
    }
    const composite_subscription& get_subscription() const {
        return lifetime;
    }
    composite_subscription& get_subscription() {
        return lifetime;
    }

    // observer
    //
    template<class V>
    void on_next(V&& v) const {
        if (!is_subscribed()) {
            return;
        }
        detacher protect(this);
        destination.on_next(std::forward<V>(v));
        protect.that = nullptr;
    }
    void on_error(std::exception_ptr e) const {
        if (!is_subscribed()) {
            return;
        }
        detacher protect(this);
        destination.on_error(e);
    }
    void on_completed() const {
        if (!is_subscribed()) {
            return;
        }
        detacher protect(this);
        destination.on_completed();
    }

    // composite_subscription
    //
    bool is_subscribed() const {
        return lifetime.is_subscribed();
    }
    weak_subscription add(subscription s) const {
        return lifetime.add(std::move(s));
    }
    template<class F>
    auto add(F f) const
    -> typename std::enable_if<detail::is_unsubscribe_function<F>::value, weak_subscription>::type {
        return lifetime.add(make_subscription(std::move(f)));
    }
    void remove(weak_subscription w) const {
        return lifetime.remove(std::move(w));
    }
    void clear() const {
        return lifetime.clear();
    }
    void unsubscribe() const {
        return lifetime.unsubscribe();
    }

};

template<class T, class Observer>
auto make_subscriber(
            subscriber<T,   Observer> o)
    ->      subscriber<T,   Observer> {
    return  subscriber<T,   Observer>(std::move(o));
}

// observer
//

template<class T, class I>
auto make_subscriber(
    const                   observer<T, I>& o)
    ->      subscriber<T,   observer<T, I>> {
    return  subscriber<T,   observer<T, I>>(composite_subscription(), o);
}
template<class T, class Observer>
auto make_subscriber(const Observer& o)
    -> typename std::enable_if<
        is_observer<Observer>::value,
            subscriber<T,   Observer>>::type {
    return  subscriber<T,   Observer>(composite_subscription(), o);
}
template<class T, class OnNext>
auto make_subscriber(const OnNext& on)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value,
            subscriber<T,   observer<T, static_observer<T, OnNext>>>>::type {
    return  subscriber<T,   observer<T, static_observer<T, OnNext>>>(composite_subscription(),
                            observer<T, static_observer<T, OnNext>>(
                                        static_observer<T, OnNext>(on)));
}
template<class T, class OnNext, class OnError>
auto make_subscriber(const OnNext& on, const OnError& oe)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_error<OnError>::value,
            subscriber<T,   observer<T, static_observer<T, OnNext, OnError>>>>::type {
    return  subscriber<T,   observer<T, static_observer<T, OnNext, OnError>>>(composite_subscription(),
                            observer<T, static_observer<T, OnNext, OnError>>(
                                        static_observer<T, OnNext, OnError>(on, oe)));
}
template<class T, class OnNext, class OnCompleted>
auto make_subscriber(const OnNext& on, const OnCompleted& oc)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_completed<OnCompleted>::value,
            subscriber<T,   observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, OnCompleted>>>>::type {
    return  subscriber<T,   observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, OnCompleted>>>(composite_subscription(),
                            observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, OnCompleted>>(
                                        static_observer<T, OnNext, detail::OnErrorEmpty, OnCompleted>(on, detail::OnErrorEmpty(), oc)));
}
template<class T, class OnNext, class OnError, class OnCompleted>
auto make_subscriber(const OnNext& on, const OnError& oe, const OnCompleted& oc)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_error<OnError>::value &&
        detail::is_on_completed<OnCompleted>::value,
            subscriber<T,   observer<T, static_observer<T, OnNext, OnError, OnCompleted>>>>::type {
    return  subscriber<T,   observer<T, static_observer<T, OnNext, OnError, OnCompleted>>>(composite_subscription(),
                            observer<T, static_observer<T, OnNext, OnError, OnCompleted>>(
                                        static_observer<T, OnNext, OnError, OnCompleted>(on, oe, oc)));
}

// explicit lifetime
//
template<class T, class I>
auto make_subscriber(const composite_subscription& cs,
    const                   observer<T, I>& o)
    ->      subscriber<T,   observer<T, I>> {
    return  subscriber<T,   observer<T, I>>(cs, o);
}
template<class T, class Observer>
auto make_subscriber(const composite_subscription& cs, const Observer& o)
    -> typename std::enable_if<
        is_observer<Observer>::value,
            subscriber<T,   Observer>>::type {
    return  subscriber<T,   Observer>(cs, o);
}
template<class T, class OnNext>
auto make_subscriber(const composite_subscription& cs, const OnNext& on)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value,
            subscriber<T,   observer<T, static_observer<T, OnNext>>>>::type {
    return  subscriber<T,   observer<T, static_observer<T, OnNext>>>(cs,
                            observer<T, static_observer<T, OnNext>>(
                                        static_observer<T, OnNext>(on)));
}
template<class T, class OnNext, class OnError>
auto make_subscriber(const composite_subscription& cs, const OnNext& on, const OnError& oe)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_error<OnError>::value,
            subscriber<T,   observer<T, static_observer<T, OnNext, OnError>>>>::type {
    return  subscriber<T,   observer<T, static_observer<T, OnNext, OnError>>>(cs,
                            observer<T, static_observer<T, OnNext, OnError>>(
                                        static_observer<T, OnNext, OnError>(on, oe)));
}
template<class T, class OnNext, class OnCompleted>
auto make_subscriber(const composite_subscription& cs, const OnNext& on, const OnCompleted& oc)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_completed<OnCompleted>::value,
            subscriber<T,   observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, OnCompleted>>>>::type {
    return  subscriber<T,   observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, OnCompleted>>>(cs,
                            observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, OnCompleted>>(
                                        static_observer<T, OnNext, detail::OnErrorEmpty, OnCompleted>(on, detail::OnErrorEmpty(), oc)));
}
template<class T, class OnNext, class OnError, class OnCompleted>
auto make_subscriber(const composite_subscription& cs, const OnNext& on, const OnError& oe, const OnCompleted& oc)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_error<OnError>::value &&
        detail::is_on_completed<OnCompleted>::value,
            subscriber<T,   observer<T, static_observer<T, OnNext, OnError, OnCompleted>>>>::type {
    return  subscriber<T,   observer<T, static_observer<T, OnNext, OnError, OnCompleted>>>(cs,
                            observer<T, static_observer<T, OnNext, OnError, OnCompleted>>(
                                        static_observer<T, OnNext, OnError, OnCompleted>(on, oe, oc)));
}

// chain defaults from subscriber
//
template<class T, class OtherT, class OtherObserver, class I>
auto make_subscriber(const subscriber<OtherT, OtherObserver>& scbr,
    const                   observer<T, I>& o)
    ->      subscriber<T,   observer<T, I>> {
    return  subscriber<T,   observer<T, I>>(scbr.get_subscription(), o);
}
template<class T, class OtherT, class OtherObserver, class Observer>
auto make_subscriber(const subscriber<OtherT, OtherObserver>& scbr, const Observer& o)
    -> typename std::enable_if<
        is_observer<Observer>::value,
            subscriber<T,   Observer>>::type {
    return  subscriber<T,   Observer>(scbr.get_subscription(), o);
}
template<class T, class OtherT, class OtherObserver, class OnNext>
auto make_subscriber(const subscriber<OtherT, OtherObserver>& scbr, const OnNext& on)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value,
            subscriber<T,   observer<T, static_observer<T, OnNext>>>>::type {
    return  subscriber<T,   observer<T, static_observer<T, OnNext>>>(scbr.get_subscription(),
                            observer<T, static_observer<T, OnNext>>(
                                        static_observer<T, OnNext>(on)));
}
template<class T, class OtherT, class OtherObserver, class OnNext, class OnError>
auto make_subscriber(const subscriber<OtherT, OtherObserver>& scbr, const OnNext& on, const OnError& oe)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_error<OnError>::value,
            subscriber<T,   observer<T, static_observer<T, OnNext, OnError>>>>::type {
    return  subscriber<T,   observer<T, static_observer<T, OnNext, OnError>>>(scbr.get_subscription(),
                            observer<T, static_observer<T, OnNext, OnError>>(
                                        static_observer<T, OnNext, OnError>(on, oe)));
}
template<class T, class OtherT, class OtherObserver, class OnNext, class OnCompleted>
auto make_subscriber(const subscriber<OtherT, OtherObserver>& scbr, const OnNext& on, const OnCompleted& oc)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_completed<OnCompleted>::value,
            subscriber<T,   observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, OnCompleted>>>>::type {
    return  subscriber<T,   observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, OnCompleted>>>(scbr.get_subscription(),
                            observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, OnCompleted>>(
                                        static_observer<T, OnNext, detail::OnErrorEmpty, OnCompleted>(on, detail::OnErrorEmpty(), oc)));
}
template<class T, class OtherT, class OtherObserver, class OnNext, class OnError, class OnCompleted>
auto make_subscriber(const subscriber<OtherT, OtherObserver>& scbr, const OnNext& on, const OnError& oe, const OnCompleted& oc)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_error<OnError>::value &&
        detail::is_on_completed<OnCompleted>::value,
            subscriber<T,   observer<T, static_observer<T, OnNext, OnError, OnCompleted>>>>::type {
    return  subscriber<T,   observer<T, static_observer<T, OnNext, OnError, OnCompleted>>>(scbr.get_subscription(),
                            observer<T, static_observer<T, OnNext, OnError, OnCompleted>>(
                                        static_observer<T, OnNext, OnError, OnCompleted>(on, oe, oc)));
}

template<class T, class OtherT, class OtherObserver, class I>
auto make_subscriber(const subscriber<OtherT, OtherObserver>& scbr, const composite_subscription& cs,
    const                   observer<T, I>& o)
    ->      subscriber<T,   observer<T, I>> {
    return  subscriber<T,   observer<T, I>>(cs, o);
}
template<class T, class OtherT, class OtherObserver, class Observer>
auto make_subscriber(const subscriber<OtherT, OtherObserver>& scbr, const composite_subscription& cs, const Observer& o)
    -> typename std::enable_if<
        is_observer<Observer>::value,
            subscriber<T,   Observer>>::type {
    return  subscriber<T,   Observer>(cs, o);
}
template<class T, class OtherT, class OtherObserver, class OnNext>
auto make_subscriber(const subscriber<OtherT, OtherObserver>& scbr, const composite_subscription& cs, const OnNext& on)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value,
            subscriber<T,   observer<T, static_observer<T, OnNext>>>>::type {
    return  subscriber<T,   observer<T, static_observer<T, OnNext>>>(cs,
                            observer<T, static_observer<T, OnNext>>(
                                        static_observer<T, OnNext>(on)));
}
template<class T, class OtherT, class OtherObserver, class OnNext, class OnError>
auto make_subscriber(const subscriber<OtherT, OtherObserver>& scbr, const composite_subscription& cs, const OnNext& on, const OnError& oe)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_error<OnError>::value,
            subscriber<T,   observer<T, static_observer<T, OnNext, OnError>>>>::type {
    return  subscriber<T,   observer<T, static_observer<T, OnNext, OnError>>>(cs,
                            observer<T, static_observer<T, OnNext, OnError>>(
                                        static_observer<T, OnNext, OnError>(on, oe)));
}
template<class T, class OtherT, class OtherObserver, class OnNext, class OnCompleted>
auto make_subscriber(const subscriber<OtherT, OtherObserver>& scbr, const composite_subscription& cs, const OnNext& on, const OnCompleted& oc)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_completed<OnCompleted>::value,
            subscriber<T,   observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, OnCompleted>>>>::type {
    return  subscriber<T,   observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, OnCompleted>>>(cs,
                            observer<T, static_observer<T, OnNext, detail::OnErrorEmpty, OnCompleted>>(
                                        static_observer<T, OnNext, detail::OnErrorEmpty, OnCompleted>(on, detail::OnErrorEmpty(), oc)));
}
template<class T, class OtherT, class OtherObserver, class OnNext, class OnError, class OnCompleted>
auto make_subscriber(const subscriber<OtherT, OtherObserver>& scbr, const composite_subscription& cs, const OnNext& on, const OnError& oe, const OnCompleted& oc)
    -> typename std::enable_if<
        detail::is_on_next_of<T, OnNext>::value &&
        detail::is_on_error<OnError>::value &&
        detail::is_on_completed<OnCompleted>::value,
            subscriber<T,   observer<T, static_observer<T, OnNext, OnError, OnCompleted>>>>::type {
    return  subscriber<T,   observer<T, static_observer<T, OnNext, OnError, OnCompleted>>>(cs,
                            observer<T, static_observer<T, OnNext, OnError, OnCompleted>>(
                                        static_observer<T, OnNext, OnError, OnCompleted>(on, oe, oc)));
}

// override lifetime
//
template<class T, class Observer>
auto make_subscriber(const subscriber<T, Observer>& scbr, const composite_subscription& cs)
    ->      subscriber<T,   Observer> {
    return  subscriber<T,   Observer>(cs, scbr.get_observer());
}

}

#endif
