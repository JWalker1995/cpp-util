#ifndef JWUTIL_POOL_H
#define JWUTIL_POOL_H

#include <deque>
#include <vector>

namespace jw_util
{

template <typename Type>
class Pool
{
public:
    Type &alloc()
    {
        if (freed.empty())
        {
            pool.emplace_back();
            return pool.back();
        }
        else
        {
            Type &res = *freed.back();
            freed.pop_back();
            return res;
        }
    }

    void free(Type &type)
    {
        freed.push_back(&type);
    }

private:
    std::deque<Type> pool;
    std::vector<Type*> freed;
};

}

#endif // JWUTIL_POOL_H
