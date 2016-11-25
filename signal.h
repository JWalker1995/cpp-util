#ifndef JWUTIL_SIGNAL_H
#define JWUTIL_SIGNAL_H

#include <assert.h>
#include <vector>

#include "methodcallback.h"

namespace jw_util
{

/*
Fast insertion (listen) and calling
Slow removal (ignore)
No guarantee on listener call order
*/

template <unsigned int filter_arg, typename... ArgTypes>
class SignalRouter;

template <typename... ArgTypes>
class Signal
{
    template <unsigned int filter_arg, typename... ArgTypes2> friend class SignalRouter;

public:
    typedef MethodCallback<ArgTypes...> ListenerType;

    ~Signal()
    {
#ifndef NDEBUG
        destructed = true;
#endif
    }

    void listen(ListenerType method_callback)
    {
        listeners.push_back(method_callback);
    }

    template <bool keep_order = false>
    void ignore(ListenerType method_callback)
    {
        typename std::vector<ListenerType>::iterator i = listeners.begin();
        while (i != listeners.end())
        {
            if (*i == method_callback)
            {
                if (keep_order)
                {
                    listeners.erase(i);
                }
                else
                {
                    *i = std::move(listeners.back());
                    listeners.pop_back();
                }
                return;
            }

            i++;
        }

        assert(false);
    }

    bool has_listeners() const {return !listeners.empty();}

    void call(ArgTypes... args) const
    {
        typename std::vector<ListenerType>::const_iterator i = listeners.cbegin();
        while (i != listeners.cend())
        {
            i->call(std::forward<ArgTypes>(args)...);
            i++;
        }
    }

private:
    std::vector<ListenerType> listeners;

#ifndef NDEBUG
    bool destructed = false;
#endif
};

}

#endif // JWUTIL_SIGNAL_H
