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
    Type *alloc()
    {
        if (freed.empty())
        {
            pool.emplace_back();
            return &pool.back();
        }
        else
        {
            Type *res = freed.back();
            freed.pop_back();
            return res;
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
            freed.push_back(const_cast<Type *>(type));
        }
    }

    ContainerType &get_data() {return pool;}

private:
    ContainerType pool;
    std::vector<Type *> freed;
};

}

#endif // JWUTIL_POOL_H
