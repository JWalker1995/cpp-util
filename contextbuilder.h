#ifndef JWUTIL_CONTEXTBUILDER_H
#define JWUTIL_CONTEXTBUILDER_H

#include "context.h"

namespace jw_util {

class ContextBuilder {
public:
    static ContextBuilder &getInstance() {
        static ContextBuilder instance;
        return instance;
    }

    template <typename InterfaceType, typename ImplementationType = InterfaceType>
    int provide() {
        provisions.push_back(&provideTo<InterfaceType, ImplementationType>);
        return 0;
    }

    void buildTo(jw_util::Context &context) {
        std::vector<void (*)(jw_util::Context &context)>::const_iterator i = provisions.cbegin();
        while (i != provisions.cend()) {
            (**i)(context);
            i++;
        }
    }

private:
    std::vector<void (*)(jw_util::Context &context)> provisions;

    template <typename InterfaceType, typename ImplementationType>
    static void provideTo(jw_util::Context &context) {
        context.provide<InterfaceType, ImplementationType>();
    }
};

}

#endif // JWUTIL_CONTEXTBUILDER_H
