#ifndef JWUTIL_CACHELRU_H
#define JWUTIL_CACHELRU_H

#include <unordered_map>
#include <assert.h>

namespace jw_util
{

template <typename KeyType, typename ValueType, unsigned int num_buckets, typename Hasher = std::hash<KeyType>>
class CacheLRU
{
private:
    struct ForgetNode;

public:
    CacheLRU()
        : map(num_buckets)
    {
        map.max_load_factor(1.0f);

        forget_pool = new ForgetNode[num_buckets];
        forget_pool_back = forget_pool;
    }

    ~CacheLRU()
    {
        delete[] forget_pool;
    }

    class Result
    {
        friend class CacheLRU;

    public:
        ValueType *get_value() const {return &bucket->first;}
        bool is_valid() const {return valid;}

    private:
        std::pair<ValueType, ForgetNode *> *bucket;
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
        res.bucket = &element.first->second;
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

    unsigned int get_bucket_id(const Result &result) const
    {
        assert(result.bucket->second >= forget_pool);
        assert(result.bucket->second < forget_pool_back);

        unsigned int res = result.bucket->second - forget_pool;
        assert(res < num_buckets);
        return res;
    }

    const KeyType &lookup_bucket(unsigned int id) const
    {
        assert(id < num_buckets);

        ForgetNode *node = forget_pool + id;
        assert(node < forget_pool_back);
        return node->val->first;
    }

private:
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
            assert(forget_pool_back < forget_pool + num_buckets);
            node = forget_pool_back;
            forget_pool_back++;
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
    ForgetNode *forget_pool;
    ForgetNode *forget_pool_back;
};

}

#endif // JWUTIL_CACHELRU_H
