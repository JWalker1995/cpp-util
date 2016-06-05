#ifndef JWUTIL_HASH_H
#define JWUTIL_HASH_H

#include <cstddef>

namespace jw_util
{

class Hash
{
public:
    static constexpr std::size_t combine(std::size_t hash, std::size_t val)
    {
        return hash ^ (val + 0x9e3779b9 + (hash << 6) + (hash >> 2));
    }
};

}

#endif // JWUTIL_HASH_H
