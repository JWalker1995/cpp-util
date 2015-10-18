#ifndef METHODCALLBACK_H
#define METHODCALLBACK_H

#include <assert.h>
#include <utility>

namespace jw_util
{

template <typename... ArgTypes>
class MethodCallback
{
public:
    template <void (*Function)(ArgTypes...)>
    static MethodCallback<ArgTypes...> create()
    {
        return MethodCallback(static_cast<void*>(0), &function_stub<Function>);
    }

    template <typename ClassType, void (ClassType::*Method)(ArgTypes...)>
    static MethodCallback<ArgTypes...> create(ClassType *inst)
    {
        return MethodCallback(static_cast<void*>(inst), &method_stub<ClassType, Method>);
    }

    void call(ArgTypes... args) const
    {
        assert(inst_ptr);
        (*stub_ptr)(inst_ptr, std::forward<ArgTypes>(args)...);
    }

private:
    typedef void (*StubType)(void *inst_ptr, ArgTypes...);

    MethodCallback()
        : inst_ptr(0)
    {}

    MethodCallback(void *inst_ptr, StubType stub_ptr)
        : inst_ptr(inst_ptr)
        , stub_ptr(stub_ptr)
    {}

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

    void *inst_ptr;
    StubType stub_ptr;
};

}

#endif // METHODCALLBACK_H
