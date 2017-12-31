#ifndef JWUTIL_POOL_H
#define JWUTIL_POOL_H

#include <deque>
#include <vector>

namespace jw_util
{

template <typename Type, bool shrink = false, typename ContainerType = std::deque<Type>>
class Pool
{
public:
    template <typename... ArgTypes>
    Type *alloc(ArgTypes &&... args)
    {
        if (freed.empty())
        {
            pool.emplace_back(std::forward<ArgTypes>(args)...);
            return &pool.back();
        }
        else
        {
            Type *res = freed.back();
            freed.pop_back();
            return new (res) Type(std::forward<ArgTypes>(args)...);
        }
    }

    void free(const Type *type)
    {
        if (shrink && type == &pool.back())
        {
            pool.pop_back();
        }
        else
        {
            type->Type::~Type();
            freed.push_back(const_cast<Type *>(type));
        }
    }

    const ContainerType &get_data() const {return pool;}
    ContainerType &get_data() {return pool;}

private:
    ContainerType pool;
    std::vector<Type *> freed;
};

}

#endif // JWUTIL_POOL_H
