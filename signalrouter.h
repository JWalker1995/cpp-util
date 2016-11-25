#ifndef JWUTIL_SIGNALROUTER_H
#define JWUTIL_SIGNALROUTER_H

#include <assert.h>
#include <vector>
#include <tuple>

#include "signal.h"

namespace jw_util
{

template <unsigned int filter_arg, typename... ArgTypes>
class SignalRouter<filter_arg, Signal<ArgTypes...>>
{
private:
    typedef Signal<ArgTypes...> SignalType;
    typedef typename SignalType::ListenerType ListenerType;
    typedef SignalRouter<filter_arg, SignalType> ThisType;
    typedef typename std::tuple_element<filter_arg, std::tuple<ArgTypes...>>::type FilterArgType;

public:
    SignalRouter(SignalType &signal)
        : signal(signal)
    {}

    ~SignalRouter()
    {
#ifndef NDEBUG
        // SignalRouters must be destructed before their Signals
        assert(!signal.destructed);
#endif

        typename std::vector<ListenerType>::iterator i = signal.listeners.begin();
        while (i != signal.listeners.end())
        {
            if (i->is_same_method(ListenerType::template create<RouteTable, &RouteTable::callback>(0)))
            {
                RouteTable *inst = i->template get_inst<RouteTable>();
                if (inst->router == this)
                {
                    *i = std::move(signal.listeners.back());
                    signal.listeners.pop_back();
                    i--;
                    delete inst;
                }
            }
            i++;
        }
    }

    void listen(FilterArgType filter, ListenerType callback)
    {
        typename std::vector<ListenerType>::const_iterator i = signal.listeners.cbegin();
        while (i != signal.listeners.cend())
        {
            if (i->is_same_method(ListenerType::template create<RouteTable, &RouteTable::callback>(0)))
            {
                ListenerType &existing = i->template get_inst<RouteTable>()->at(filter);
                if (!existing.is_valid())
                {
                    existing = callback;
                    return;
                }
            }
            i++;
        }

        RouteTable *route_table = new RouteTable(this);
        signal.listen(ListenerType::template create<RouteTable, &RouteTable::callback>(route_table));

        route_table->at(filter) = callback;
    }

    void ignore(FilterArgType filter, ListenerType callback)
    {
        typename std::vector<ListenerType>::const_iterator i = signal.listeners.cbegin();
        while (i != signal.listeners.cend())
        {
            if (i->is_same_method(ListenerType::template create<RouteTable, &RouteTable::callback>(0)))
            {
                RouteTable *inst = i->template get_inst<RouteTable>();
                if (filter < inst->table.size())
                {
                    if (inst->table[filter] == callback)
                    {
                        inst->table[filter] = ListenerType();
                        return;
                    }
                }
            }
            i++;
        }
    }

private:
    struct RouteTable
    {
        RouteTable(const ThisType *router)
            : router(router)
        {}

        ListenerType &at(FilterArgType filter)
        {
            if (table.size() <= filter)
            {
                table.resize(filter + 1);
            }
            return table[filter];
        }

        void callback(ArgTypes... args)
        {
            FilterArgType dst = std::get<filter_arg>(std::tuple<ArgTypes...>(std::forward<ArgTypes>(args)...));
            if (dst < table.size())
            {
                ListenerType &listener = table[dst];
                if (listener.is_valid())
                {
                    listener.call(std::forward<ArgTypes>(args)...);
                }
            }
        }

        const ThisType *router;
        std::vector<ListenerType> table;
    };

    SignalType &signal;
};

}

#endif // JWUTIL_SIGNALROUTER_H
