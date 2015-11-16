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
    typedef typename SignalType::Listener ListenerType;
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

        unsigned int i = 0;
        while (i < signal.listeners.size())
        {
            ListenerType &listener = signal.listeners[i];
            if (listener.stub_ptr == &RouteTable::callback)
            {
                RouteTable *inst = static_cast<RouteTable*>(listener.inst_ptr);
                if (inst->router == this)
                {
                    ListenerType &back = signal.listeners.back();
                    if (&listener != &back)
                    {
                        listener = std::move(back);
                    }
                    signal.listeners.pop_back();
                    i--;
                    delete inst;
                }
            }
            i++;
        }
    }

    template <void (*Function)(ArgTypes...)>
    void listen(FilterArgType filter)
    {
        add_listener(filter, static_cast<void*>(0), &SignalType::Listener::template function_stub<Function>);
    }

    template <typename ClassType, void (ClassType::*Method)(ArgTypes...)>
    void listen(FilterArgType filter, ClassType *inst)
    {
        add_listener(filter, static_cast<void*>(inst), &SignalType::Listener::template method_stub<ClassType, Method>);
    }

    template <void (*Function)(ArgTypes...)>
    void ignore(FilterArgType filter)
    {
        remove_listener(filter, static_cast<void*>(0), &SignalType::Listener::template function_stub<Function>);
    }

    template <typename ClassType, void (ClassType::*Method)(ArgTypes...)>
    void ignore(FilterArgType filter, ClassType *inst)
    {
        remove_listener(filter, static_cast<void*>(inst), &SignalType::Listener::template method_stub<ClassType, Method>);
    }

private:
    void add_listener(FilterArgType filter, void *inst_ptr, typename SignalType::StubType stub_ptr)
    {
        typename std::vector<ListenerType>::const_iterator i = signal.listeners.cbegin();
        while (i != signal.listeners.cend())
        {
            if (i->stub_ptr == &RouteTable::callback)
            {
                RouteTable &inst = *static_cast<RouteTable*>(i->inst_ptr);
                ListenerType &listener = inst.at(filter);
                if (!listener.inst_ptr)
                {
                    listener.inst_ptr = inst_ptr;
                    listener.stub_ptr = stub_ptr;
                    return;
                }
            }
            i++;
        }

        RouteTable *route_table = new RouteTable(this);
        signal.add_listener(route_table, &RouteTable::callback);

        ListenerType &listener = route_table->at(filter);
        listener.inst_ptr = inst_ptr;
        listener.stub_ptr = stub_ptr;
    }

    void remove_listener(FilterArgType filter, void *inst_ptr, typename SignalType::StubType stub_ptr)
    {
        typename std::vector<ListenerType>::const_iterator i = signal.listeners.cbegin();
        while (i != signal.listeners.cend())
        {
            if (i->stub_ptr == &RouteTable::callback)
            {
                RouteTable &inst = *static_cast<RouteTable*>(i->inst_ptr);
                if (filter < inst.table.size())
                {
                    ListenerType &listener = inst.table[filter];
                    if (listener.inst_ptr == inst_ptr && listener.stub_ptr == stub_ptr)
                    {
                        listener.inst_ptr = 0;
                        return;
                    }
                }
            }
            i++;
        }

        assert(false);
    }

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

        static void callback(void *inst_ptr, ArgTypes... args)
        {
            RouteTable* inst = static_cast<RouteTable*>(inst_ptr);

            FilterArgType dst = std::get<filter_arg>(std::tuple<ArgTypes...>(std::forward<ArgTypes>(args)...));
            if (dst < inst->table.size())
            {
                ListenerType &listener = inst->table[dst];
                if (listener.inst_ptr)
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
