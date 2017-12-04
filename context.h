#ifndef JWUTIL_CONTEXT_H
#define JWUTIL_CONTEXT_H

#include <tuple>

namespace {

template<int Index, typename Search, typename First, typename... Types>
struct get_internal {
    typedef typename get_internal<Index + 1, Search, Types...>::type type;
    static constexpr int index = Index;
};

template<int Index, typename Search, typename... Types>
struct get_internal<Index, Search, Search, Types...> {
    typedef get_internal type;
    static constexpr int index = Index;
};

template<typename GetType, typename... Types>
GetType getFirst(std::tuple<Types...> tuple) {
    return std::get<get_internal<0, GetType, Types...>::type::index>(tuple);
}

}

namespace jw_util {

template <typename... ArgTypes>
class Context {
    template <typename... FriendArgTypes>
    friend class Context;

public:
    Context(ArgTypes &... newArgs)
        : refs(newArgs...)
    {}

    template <typename... ParentContextArgs, typename... NewArgTypes>
    Context(const Context<ParentContextArgs...> &parentContext, NewArgTypes &... newArgs)
        : refs(getFirst<ArgTypes &>(std::tuple_cat(std::tuple<NewArgTypes &...>(newArgs...), parentContext.refs))...)
    {}

    template <typename GetType>
    GetType &get() const {
        return std::get<GetType &>(refs);
    }

    template <typename... NewArgTypes>
    Context<ArgTypes..., NewArgTypes...> extend(NewArgTypes &... newArgs) const {
        return Context<ArgTypes..., NewArgTypes...>(*this, newArgs...);
    }

private:
    std::tuple<ArgTypes &...> refs;
};

}

#endif // JWUTIL_CONTEXT_H
