#ifndef JWUTIL_REGISTRY_H
#define JWUTIL_REGISTRY_H

#include <unordered_map>

#include "jw_util/baseexception.h"
#include "jw_util/config.h"
#include "jw_util/context.h"

namespace jw_util {

template <typename ElementType>
class Registry {
public:
    class AccessException : public BaseException {
    public:
        AccessException(const std::string &key)
            : BaseException("Could not find key \"" + key + "\" in registry")
        {}
    };

    template <typename... ContextArgs>
    Registry(Context<ContextArgs...> context) {
        Config::MapIterator i(context.template get<const Config>());
        while (i.has()) {
            auto res = map.emplace(i.key(), context.extend(i.val()));
            assert(res.second);
            i.advance();
        }
    }

    ElementType &get(const std::string &key) {
        typename std::unordered_map<std::string, ElementType>::const_iterator found = map.find(key);
        if (found != map.cend()) {
            return map;
        } else {
            throw AccessException(key);
        }
    }

    std::unordered_map<std::string, ElementType> &getMap() {
        return map;
    }

private:
    std::unordered_map<std::string, ElementType> map;
};

}

#endif // JWUTIL_REGISTRY_H
