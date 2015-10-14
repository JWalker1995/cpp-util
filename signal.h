#ifndef SIGNAL_H
#define SIGNAL_H

#include <assert.h>
#include <vector>

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
    ~Signal()
    {
#ifndef NDEBUG
        destructed = true;
#endif
    }

    template <void (*Function)(ArgTypes...)>
    void listen()
    {
        add_listener(static_cast<void*>(0), &Listener::template function_stub<Function>);
    }

    template <typename ClassType, void (ClassType::*Method)(ArgTypes...)>
    void listen(ClassType *inst)
    {
        add_listener(static_cast<void*>(inst), &Listener::template method_stub<ClassType, Method>);
    }

    template <void (*Function)(ArgTypes...)>
    void ignore()
    {
        remove_listener(static_cast<void*>(0), &Listener::template function_stub<Function>);
    }

    template <typename ClassType, void (ClassType::*Method)(ArgTypes...)>
    void ignore(ClassType *inst)
    {
        remove_listener(static_cast<void*>(inst), &Listener::template method_stub<ClassType, Method>);
    }

    bool has_listeners() const {return !listeners.empty();}

    void call(ArgTypes... args) const
    {
        typename std::vector<Listener>::const_iterator i = listeners.cbegin();
        while (i != listeners.cend())
        {
            i->call(std::forward<ArgTypes>(args)...);
            i++;
        }
    }

private:
    typedef void (*StubType)(void *inst_ptr, ArgTypes...);

    struct Listener
    {
        Listener()
            : inst_ptr(0)
        {}

        Listener(void *inst_ptr, StubType stub_ptr)
            : inst_ptr(inst_ptr)
            , stub_ptr(stub_ptr)
        {}

        void call(ArgTypes... args) const
        {
            assert(inst_ptr);
            (*stub_ptr)(inst_ptr, std::forward<ArgTypes>(args)...);
        }

        void *inst_ptr;
        StubType stub_ptr;

        template <void (*Function)(ArgTypes...)>
        static void function_stub(void *inst_ptr, ArgTypes... args)
        {
            (void) inst_ptr;
            (*Function)(std::forward<ArgTypes>(args)...);
        }

        template <class ClassType, void (ClassType::*Method)(ArgTypes...)>
        static void method_stub(void *inst_ptr, ArgTypes... args)
        {
            ClassType* inst = static_cast<ClassType*>(inst_ptr);
            (inst->*Method)(std::forward<ArgTypes>(args)...);
        }
    };

    void add_listener(void *inst_ptr, StubType stub_ptr)
    {
        listeners.emplace_back(inst_ptr, stub_ptr);
    }

    void remove_listener(void *inst_ptr, StubType stub_ptr)
    {
        typename std::vector<Listener>::iterator i = listeners.begin();
        while (i != listeners.end())
        {
            if (i->inst_ptr == inst_ptr && i->stub_ptr == stub_ptr)
            {
                Listener &back = listeners.back();
                if (&*i != &back)
                {
                    *i = back;
                }
                listeners.pop_back();
                return;
            }
            i++;
        }

        assert(false);
    }

    std::vector<Listener> listeners;

#ifndef NDEBUG
    bool destructed = false;
#endif
};

}

#endif // SIGNAL_H
