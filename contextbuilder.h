#ifndef JWUTIL_CONTEXTBUILDER_H
#define JWUTIL_CONTEXTBUILDER_H

#include <vector>
#include <algorithm>

namespace jw_util {

template <typename ContextType>
class ContextBuilder {
public:
    template <typename InterfaceType, typename ImplementationType = InterfaceType>
    int registerConstructor() {
        constructors.push_back(&construct<InterfaceType, ImplementationType>);
        return 0;
    }

    template <typename InterfaceType, typename ImplementationType = InterfaceType>
    void retract() {
        typename std::vector<void (*)(ContextType &context)>::iterator pos = std::find(constructors.begin(), constructors.end(), &construct<InterfaceType, ImplementationType>);
        assert(pos != constructors.end());
        constructors.erase(pos);
    }

    void buildAll(ContextType &context) {
        typename std::vector<void (*)(ContextType &context)>::const_iterator i = constructors.cbegin();
        while (i != constructors.cend()) {
            (**i)(context);
            i++;
        }
    }

    std::size_t getSize() const {
        return constructors.size();
    }

private:
    std::vector<void (*)(ContextType &context)> constructors;

    template <typename InterfaceType, typename ImplementationType>
    static void construct(ContextType &context) {
        context.template construct<InterfaceType, ImplementationType>();
    }
};

}

#endif // JWUTIL_CONTEXTBUILDER_H
