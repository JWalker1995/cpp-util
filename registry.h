#ifndef JWUTIL_REGISTRY_H
#define JWUTIL_REGISTRY_H

#include <unordered_map>

#include "jw_util/config.h"
#include "jw_util/context.h"

namespace jw_util {

template <typename ElementType>
class Registry {
public:
    class AccessException : public Exception {
    public:
        AccessException(const std::string &key)
            : Exception("Could not find key \"" + key + "\" in registry")
        {}
    };

    template <typename... ContextArgs>
    Registry(Context<ContextArgs...> context) {
        Config::const_iterator i = context.get<Config>().cbegin();
        while (i != context.get<Config>().cend()) {
            map.emplace(i->first, context.extend(Config(i->second)));
            i++;
        }
    }

    ElementType &get(const std::string &key) {
        std::unordered_map<std::string, ElementType>::const_iterator found = map.find(key);
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
