#ifndef JWUTIL_CONTEXTBUILDER_H
#define JWUTIL_CONTEXTBUILDER_H

#include "context.h"

namespace jw_util {

class ContextBuilder {
public:
    template <typename InterfaceType, typename ImplementationType = InterfaceType>
    static int provide() {
        provisions.push_back(&provideTo<InterfaceType, ImplementationType>);
        return 0;
    }

    static void buildTo(jw_util::Context &context) {
        std::vector<void (*)(jw_util::Context &context)>::const_iterator i = provisions.cbegin();
        while (i != provisions.cend()) {
            (**i)(context);
            i++;
        }
    }

private:
    static std::vector<void (*)(jw_util::Context &context)> provisions;

    template <typename InterfaceType, typename ImplementationType>
    static void provideTo(jw_util::Context &context) {
        context.provide<InterfaceType, ImplementationType>();
    }
};

}

#endif // JWUTIL_CONTEXTBUILDER_H
