#ifndef JWUTIL_CACHELRU_H
#define JWUTIL_CACHELRU_H

#include <deque>
#include <unordered_map>
#include <assert.h>

namespace jw_util
{

template <typename KeyType, typename ValueType, unsigned int num_buckets, typename Hasher = std::hash<KeyType>>
class CacheLRU
{
public:
    CacheLRU()
        : map(num_buckets)
    {
        map.max_load_factor(1.0f);
    }

    struct Result
    {
        ValueType *value;
        bool valid;
    };

    Result access(const KeyType &key)
    {
        float next_load_factor = (map.size() + 2) / map.bucket_count();
        if (next_load_factor > map.max_load_factor())
        {
            map.erase(forget_get_front());
            forget_shift();
        }

        std::pair<KeyType, std::pair<ValueType, ForgetNode *>> insert;
        insert.first = key;
        std::pair<typename MapType::iterator, bool> element = map.insert(std::move(insert));

        Result res;
        res.value = &element.first->second.first;
        res.valid = !element.second;

        ForgetNode *node;
        if (res.valid)
        {
            // Found element in cache
            node = element.first->second.second;
            forget_erase(node);
        }
        else
        {
            // Did not find, inserted element into cache
            node = forget_alloc();
            node->val = element.first;
            element.first->second.second = node;
        }
        forget_push_back(node);

        return res;
    }

private:
    struct ForgetNode;
    typedef std::unordered_map<KeyType, std::pair<ValueType, ForgetNode *>, Hasher> MapType;

    struct ForgetNode
    {
        ForgetNode *prev = 0;
        ForgetNode *next = 0;
        typename MapType::const_iterator val;
    };

    ForgetNode *forget_alloc()
    {
        ForgetNode *node;
        if (forget_available)
        {
            node = forget_available;
            forget_available = 0;
        }
        else
        {
            forget_pool.emplace_back();
            node = &forget_pool.back();
        }
        return node;
    }

    typename MapType::const_iterator forget_get_front() const
    {
        return forget_front->val;
    }

    void forget_shift()
    {
#ifndef NDEBUG
        assert(forget_available == 0);
#endif
        forget_available = forget_front;
        forget_front = forget_front->next;
    }

    void forget_erase(ForgetNode *node)
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    void forget_push_back(ForgetNode *node)
    {
        forget_back->next = node;
        node->prev = forget_back;
        forget_back = node;
    }

    MapType map;
    ForgetNode *forget_front = 0;
    ForgetNode *forget_back = 0;
    ForgetNode *forget_available = 0;
    std::deque<ForgetNode> forget_pool;
};

}

#endif // JWUTIL_CACHELRU_H
