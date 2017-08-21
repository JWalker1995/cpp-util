#ifndef JWUTIL_CONTEXT_H
#define JWUTIL_CONTEXT_H

#include <tuple>

namespace jw_util {

template <typename... ArgTypes>
class Context {
    template <typename... FriendArgTypes>
    friend class Context;

public:
    template <typename... NewArgTypes>
    Context(NewArgTypes &... newArgs)
        : refs {newArgs...}
    {}

    template <typename... ParentContextArgs, typename... NewArgTypes>
    Context(Context<ParentContextArgs...> &&parentContext, NewArgTypes &... newArgs)
        : refs {std::get<ArgTypes &>(std::tuple_cat(parentContext.refs, std::tuple<NewArgTypes &...>(newArgs...)))...}
    {}

    template <typename GetType>
    GetType &get() {
        return std::get<GetType &>(refs);
    }

private:
    std::tuple<ArgTypes&...> refs;
};

}

#endif // JWUTIL_CONTEXT_H
