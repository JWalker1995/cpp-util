#ifndef JWUTIL_CONTEXT_H
#define JWUTIL_CONTEXT_H

#include <assert.h>
#include <iostream>
#include <typeindex>
#include <vector>
#include <unordered_map>

#define DEBUG_CONTEXT 1

namespace jw_util {

class Context {
public:
    Context() {}
    Context(const Context& other) = delete;
    Context &operator=(const Context &other) = delete;

    template <typename InterfaceType, typename ImplementationType = InterfaceType>
    void provide() {
        if (DEBUG_CONTEXT) {
            std::cout << "Context::registerClass: " << typeid(InterfaceType).name() << ", " << typeid(ImplementationType).name() << std::endl;
        }
        ClassEntry entry;
        entry.prepareManagedInstance<InterfaceType, ImplementationType>();
        auto inserted = classMap.emplace(std::type_index(typeid(InterfaceType)), entry);
        assert(inserted.second);
    }

    template <typename InterfaceType>
    void provideInstance(InterfaceType &instance) {
        if (DEBUG_CONTEXT) {
            std::cout << "Context::insertInstance: " << typeid(InterfaceType).name() << std::endl;
        }
        ClassEntry entry;
        entry.setBorrowedInstance<InterfaceType>(&instance);
        auto inserted = classMap.emplace(std::type_index(typeid(InterfaceType)), entry);
        assert(inserted.second);
    }

    template <typename InterfaceType>
    InterfaceType &get() {
        if (DEBUG_CONTEXT) {
            std::cout << "Context::get: " << typeid(InterfaceType).name() << std::endl;
        }
        auto found = classMap.find(std::type_index(typeid(InterfaceType)));
        if (found == classMap.end()) {
            std::cout << "Context::get: Cannot find any provided type with interface " << typeid(InterfaceType).name() << std::endl;
            assert(false);

            /*
            // Maybe the type is registered/inserted by a class that hasn't been instantiated yet
            createSomething();
            return get<InterfaceType>();
            */
        }
        if (!found->second.hasInstance()) {
            found->second.createManagedInstance(*this);
            classOrder.push_back(&found->second);
        }
        return *found->second.getInstance<InterfaceType>();
    }

    /*
    void createSomething() {
        // This method is kind of hacky
        assert(false);

        assert(!classMap.empty());

        // Any iteration over a std::unordered_map will be in an implementation-dependent order,
        // So we explicitly randomize it here so we don't get weird edge cases.
        static std::random_device randomDevice;
        static std::mt19937 randomEngine(randomDevice());
        unsigned int index = std::uniform_int_distribution<unsigned int>(0, classMap.size() - 1)(randomEngine);
        if (DEBUG_CONTEXT) {
            std::cout << "Context::createSomething: rand(" << classMap.size() << ") -> " << index << std::endl;
        }

        auto offset = std::next(classMap.begin(), index);
        auto i = offset;
        do {
            if (!i->second.hasInstance()) {
                i->second.createManagedInstance(*this);
                classOrder.push_back(&i->second);
                return;
            }

            i++;
            if (i == classMap.end()) {
                i = classMap.begin();
            }
        } while (i != offset);

        // If we get here, there is a circular dependency
        assert(false);
    }
    */

    ~Context() {
        std::vector<ClassEntry *>::reverse_iterator i = classOrder.rbegin();
        while (i != classOrder.rend()) {
            ClassEntry &entry = **i;
            entry.release();
            i++;
        }
    }

private:
    class ClassEntry {
    public:
        bool hasInstance() const {
            return returnInstance;
        }

        template <typename ClassType>
        ClassType *getInstance() const {
            return static_cast<ClassType *>(returnInstance);
        }

        template <typename ClassType>
        void setBorrowedInstance(ClassType *instance) {
            assert(!createPtr);
            assert(!returnInstance);
            assert(!managedInstance);
            assert(!destroyPtr);

            managedInstance = 0;
            returnInstance = instance;
            createPtr = &ClassEntry::error;
            destroyPtr = &ClassEntry::noop;
        }

        template <typename ReturnType, typename ManagedType>
        void prepareManagedInstance() {
            assert(!createPtr);
            assert(!returnInstance);
            assert(!managedInstance);
            assert(!destroyPtr);

            createPtr = &ClassEntry::createStub<ReturnType, ManagedType>;
            destroyPtr = &ClassEntry::destroyStub<ReturnType, ManagedType>;
        }

        void createManagedInstance(Context &context) {
            assert(createPtr);
            assert(!returnInstance);
            assert(!managedInstance);
            assert(destroyPtr);

            (this->*createPtr)(context);
            createPtr = 0;
        }

        void release() {
            (this->*destroyPtr)();
        }

    private:
        void (ClassEntry::*createPtr)(Context &context) = 0;
        void *returnInstance = 0;
        void *managedInstance = 0;
        void (ClassEntry::*destroyPtr)() = 0;

        template <typename ReturnType, typename ManagedType>
        void createStub(Context &context) {
            if (DEBUG_CONTEXT) {
                std::cout << "Context::createStub: " << typeid(ReturnType).name() << ", " << typeid(ManagedType).name() << std::endl;
            }
            assert(!managedInstance);
            assert(!returnInstance);
            ManagedType *instance = new ManagedType(context);
            managedInstance = instance;
            returnInstance = static_cast<ReturnType *>(instance);
        }

        template <typename ReturnType, typename ManagedType>
        void destroyStub() {
            if (DEBUG_CONTEXT) {
                std::cout << "Context::destroyStub: " << typeid(ReturnType).name() << ", " << typeid(ManagedType).name() << std::endl;
            }
            delete static_cast<ManagedType *>(managedInstance);
            managedInstance = 0;
            returnInstance = 0;
        }

        void error(Context &context) {
            if (DEBUG_CONTEXT) {
                std::cout << "Context::error" << std::endl;
            }
            assert(false);
        }

        void noop() {}
    };

    std::vector<ClassEntry *> classOrder;
    std::unordered_map<std::type_index, ClassEntry> classMap;
};

}

#endif // JWUTIL_CONTEXT_H
