#ifndef JWUTIL_CONTEXTBUILDER_H
#define JWUTIL_CONTEXTBUILDER_H

namespace jw_util {

template <typename ContextType>
class ContextBuilder {
public:
    template <typename InterfaceType, typename ImplementationType = InterfaceType>
    int provide() {
        provisions.push_back(&provideTo<InterfaceType, ImplementationType>);
        return 0;
    }

    void buildTo(ContextType &context) {
        typename std::vector<void (*)(ContextType &context)>::const_iterator i = provisions.cbegin();
        while (i != provisions.cend()) {
            (**i)(context);
            i++;
        }
    }

private:
    std::vector<void (*)(ContextType &context)> provisions;

    template <typename InterfaceType, typename ImplementationType>
    static void provideTo(ContextType &context) {
        context.template provide<InterfaceType, ImplementationType>();
    }
};

}

#endif // JWUTIL_CONTEXTBUILDER_H
