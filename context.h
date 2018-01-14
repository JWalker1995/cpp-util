#ifndef JWUTIL_CONTEXT_H
#define JWUTIL_CONTEXT_H

#define JWUTIL_CONTEXT_ENABLE_DEBUG_INFO 1
#define JWUTIL_CONTEXT_ENABLE_DEBUG_VERBOSE 0

#if JWUTIL_CONTEXT_ENABLE_DEBUG_INFO || JWUTIL_CONTEXT_ENABLE_DEBUG_VERBOSE
#include <iostream>
#endif

#include <assert.h>
#include <typeindex>
#include <vector>
#include <unordered_map>
#include <cxxabi.h>

namespace jw_util {

class Context {
public:
    Context() {}
    Context(const Context& other) = delete;
    Context &operator=(const Context &other) = delete;

    template <typename InterfaceType, typename ImplementationType = InterfaceType>
    void provide() {
        if (JWUTIL_CONTEXT_ENABLE_DEBUG_INFO) {
            std::cout << "Context::registerClass: " << getTypeName<InterfaceType>() << ", " << getTypeName<ImplementationType>() << std::endl;
        }
        ClassEntry entry;
        entry.prepareManagedInstance<InterfaceType, ImplementationType>();
        auto inserted = classMap.emplace(std::type_index(typeid(InterfaceType)), entry);
        assert(inserted.second);
    }

    template <typename InterfaceType>
    void provideInstance(InterfaceType *instance) {
        if (JWUTIL_CONTEXT_ENABLE_DEBUG_INFO) {
            std::cout << "Context::insertInstance: " << getTypeName<InterfaceType>() << std::endl;
        }
        ClassEntry entry;
        entry.setBorrowedInstance<InterfaceType>(instance);
        auto inserted = classMap.emplace(std::type_index(typeid(InterfaceType)), entry);
        assert(inserted.second);
    }

    template <typename InterfaceType>
    InterfaceType &get() {
        if (JWUTIL_CONTEXT_ENABLE_DEBUG_VERBOSE) {
            std::cout << "Context::get: " << getTypeName<InterfaceType>() << std::endl;
        }
        auto found = classMap.find(std::type_index(typeid(InterfaceType)));
        if (found == classMap.end()) {
            if (JWUTIL_CONTEXT_ENABLE_DEBUG_INFO) {
                std::cerr << "Context::get: Cannot find any provided type with interface " << getTypeName<InterfaceType>() << std::endl;
            }
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
            assert(returnInstance);
            assert(std::is_const<ClassType>::value || !isConst);
            return const_cast<ClassType *>(static_cast<const ClassType *>(returnInstance));
        }

        template <typename ClassType>
        void setBorrowedInstance(ClassType *instance) {
            assert(!createPtr);
            assert(!returnInstance);
            assert(!managedInstance);
            assert(!destroyPtr);

            createPtr = &ClassEntry::createBorrowedInstanceError;
            returnInstance = static_cast<const void *>(instance);
            managedInstance = 0;
            isConst = std::is_const<ClassType>::value;
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
        const void *returnInstance = 0;
        const void *managedInstance = 0;
        bool isConst;
        void (ClassEntry::*destroyPtr)() = 0;

        template <typename ReturnType, typename ManagedType>
        void createStub(Context &context) {
            if (JWUTIL_CONTEXT_ENABLE_DEBUG_INFO) {
                std::cout << "Context::createStub: " << getTypeName<ReturnType>() << ", " << getTypeName<ManagedType>() << std::endl;
            }
            assert(!managedInstance);
            assert(!returnInstance);
            createPtr = &ClassEntry::doubleCreateError;
            ManagedType *instance = new ManagedType(context);
            returnInstance = static_cast<const ReturnType *>(instance);
            managedInstance = static_cast<const void *>(instance);
            isConst = std::is_const<ManagedType>::value;
        }

        template <typename ReturnType, typename ManagedType>
        void destroyStub() {
            if (JWUTIL_CONTEXT_ENABLE_DEBUG_INFO) {
                std::cout << "Context::destroyStub: " << getTypeName<ReturnType>() << ", " << getTypeName<ManagedType>() << std::endl;
            }
            delete static_cast<const ManagedType *>(managedInstance);
            returnInstance = 0;
            managedInstance = 0;
        }

        void createBorrowedInstanceError(Context &context) {
            if (JWUTIL_CONTEXT_ENABLE_DEBUG_INFO) {
                std::cout << "Context::createBorrowedInstanceError" << std::endl;
            }
            assert(false);
        }

        void doubleCreateError(Context &context) {
            if (JWUTIL_CONTEXT_ENABLE_DEBUG_INFO) {
                std::cout << "Context::doubleCreateError: This is probably because you have a circular dependency" << std::endl;
            }
            assert(false);
        }

        void noop() {}
    };

    template <typename Type>
    static const char *getTypeName() {
        int status = 0;
        static thread_local const char *realname = abi::__cxa_demangle(typeid(Type).name(), 0, 0, &status);
        assert(status == 0);
        return realname;

    }

    std::vector<ClassEntry *> classOrder;
    std::unordered_map<std::type_index, ClassEntry> classMap;
};

}

#endif // JWUTIL_CONTEXT_H
