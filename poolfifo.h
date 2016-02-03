#ifndef JWUTIL_POOLFIFO_H
#define JWUTIL_POOLFIFO_H

#include <assert.h>

namespace jw_util
{

// Thread-safe, if only one thread calls alloc and one thread calls free

template <typename Type>
class PoolFIFO
{
public:
    Type &alloc()
    {
        if (alloc_cur == alloc_end)
        {
            alloc_cur = new Type[alloc_size];
            alloc_end = alloc_cur + alloc_size;
        }
        return *alloc_cur++;
    }

    void free(Type &type)
    {
        if (free_cur)
        {
            assert(&type == free_cur);
            free_cur++;
            if (free_cur == free_end)
            {
                Type *free_start = free_end - alloc_size;
                delete[] free_start;
                free_cur = 0;
            }
        }
        else
        {
            free_cur = &type;
            free_end = free_cur + alloc_size;
        }
    }

private:
    static constexpr unsigned int alloc_bytes = 65536;
    static constexpr unsigned int alloc_size = alloc_bytes / sizeof(Type);

    Type *alloc_cur = 0;
    Type *alloc_end = 0;

    Type *free_cur = 0;
    Type *free_end;
};

}

#endif // JWUTIL_POOLFIFO_H
