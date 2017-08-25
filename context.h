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
        // TODO: Replace std::get with version that doesn't fail on duplicates
        : refs {std::get<ArgTypes &>(std::tuple_cat(std::tuple<NewArgTypes &...>(newArgs...), parentContext.refs))...}
    {}

    template <typename GetType>
    GetType &get() {
        return std::get<GetType &>(refs);
    }

    template <typename... NewArgTypes>
    Context<ArgTypes..., NewArgTypes...> extend(NewArgTypes &... newArgs) {
        return Context<ArgTypes..., NewArgTypes...>(*this, newArgs...);
    }

private:
    std::tuple<ArgTypes&...> refs;
};

}

#endif // JWUTIL_CONTEXT_H
