#ifndef JWUTIL_CONTEXT_H
#define JWUTIL_CONTEXT_H

#define JWUTIL_CONTEXT_ENABLE_DEBUG_INFO 1
#define JWUTIL_CONTEXT_ENABLE_DEBUG_VERBOSE 0
#define JWUTIL_CONTEXT_ENABLE_STRUCT_GENERATION 1

#if JWUTIL_CONTEXT_ENABLE_DEBUG_INFO || JWUTIL_CONTEXT_ENABLE_DEBUG_VERBOSE
#include <array>
#include <iostream>
#endif

#if JWUTIL_CONTEXT_ENABLE_STRUCT_GENERATION
#include <fstream>
#endif

#include <assert.h>
#include <typeindex>
#include <vector>
#include <unordered_map>
#include <cxxabi.h>

namespace jw_util {

template <typename DerivedType>
class Context {
public:
    Context(float loadFactor = 0.1f) {
        // Trade memory for speed
        classMap.max_load_factor(loadFactor);
    }

    Context(const Context& other) = delete;
    Context &operator=(const Context &other) = delete;

    Context(Context&&) = default;
    Context& operator=(Context&&) = default;


    template <typename InterfaceType, typename ImplementationType = InterfaceType, bool contextConstructor = true>
    void provide() {
        if (JWUTIL_CONTEXT_ENABLE_DEBUG_INFO) {
            logInfo(this, "Context::provide: ", getTypeName<InterfaceType>(), ", ", getTypeName<ImplementationType>());
        }

        ClassEntry entry;
        entry.template prepareManagedInstance<InterfaceType, ImplementationType, contextConstructor>();
        auto inserted = classMap.emplace(std::type_index(typeid(InterfaceType)), entry);
        assert(inserted.second);
    }

    template <typename InterfaceType>
    void retract() {
        if (JWUTIL_CONTEXT_ENABLE_DEBUG_INFO) {
            logInfo(this, "Context::retract: ", getTypeName<InterfaceType>());
        }

        auto found = classMap.find(std::type_index(typeid(InterfaceType)));
        assert(found != classMap.end());

        auto found2 = std::find(classOrder.cbegin(), classOrder.cend(), &found->second);
        assert(found2 != classOrder.cend());

        classOrder.erase(found2);

        found->second.release();
        classMap.erase(found);
    }


    template <typename InterfaceType>
    void provideInstance(InterfaceType *instance) {
        if (JWUTIL_CONTEXT_ENABLE_DEBUG_INFO) {
            logInfo(this, "Context::insertInstance: ", getTypeName<InterfaceType>());
        }
        ClassEntry entry;
        entry.template setBorrowedInstance<InterfaceType>(instance);
        auto inserted = classMap.emplace(std::type_index(typeid(InterfaceType)), entry);
        assert(inserted.second);
    }

    template <typename InterfaceType>
    InterfaceType *swapInstance(InterfaceType *newInstance) {
        if (JWUTIL_CONTEXT_ENABLE_DEBUG_INFO) {
            logInfo(this, "Context::swapInstance: ", getTypeName<InterfaceType>());
        }
        auto found = classMap.find(std::type_index(typeid(InterfaceType)));
        assert(found != classMap.end());
        return found->second.swapBorrowedInstance(newInstance);
    }

    template <typename InterfaceType>
    InterfaceType *removeInstance() {
        if (JWUTIL_CONTEXT_ENABLE_DEBUG_INFO) {
            logInfo(this, "Context::removeInstance: ", getTypeName<InterfaceType>());
        }
        auto found = classMap.find(std::type_index(typeid(InterfaceType)));
        assert(found != classMap.end());
        InterfaceType *res = found->second.swapBorrowedInstance(0);
        classMap.erase(found);
        return res;
    }


    template <typename InterfaceType>
    bool isProvided() {
        if (JWUTIL_CONTEXT_ENABLE_DEBUG_VERBOSE) {
            logInfo(this, "Context::isProvided: ", getTypeName<InterfaceType>());
        }
        return classMap.find(std::type_index(typeid(InterfaceType))) != classMap.end();
    }

    template <typename InterfaceType>
    InterfaceType &get() {
        if (JWUTIL_CONTEXT_ENABLE_DEBUG_VERBOSE) {
            logInfo(this, "Context::get: ", getTypeName<InterfaceType>());
        }
        auto found = classMap.find(std::type_index(typeid(InterfaceType)));
        if (found == classMap.end()) {
            if (JWUTIL_CONTEXT_ENABLE_DEBUG_INFO) {
                logError(this, "Context::get: Cannot find any provided type with interface ", getTypeName<InterfaceType>());
            }
            assert(false);

            /*
            // Maybe the type is registered/inserted by a class that hasn't been instantiated yet
            createSomething();
            return get<InterfaceType>();
            */
        }
        if (!found->second.hasInstance()) {
            found->second.createManagedInstance(this);
            classOrder.push_back(&found->second);
        }
        return *found->second.template getInstance<InterfaceType>();
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
            // std::cout << "Context::createSomething: rand(" << classMap.size() << ") -> " << index << std::endl;
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
#if JWUTIL_CONTEXT_ENABLE_STRUCT_GENERATION
        std::ofstream file;
        file.open(getStructFilename());
        file << generateStruct();
        file.close();
#endif

        typename std::vector<ClassEntry *>::reverse_iterator i = classOrder.rbegin();
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

#if JWUTIL_CONTEXT_ENABLE_STRUCT_GENERATION
            getCount++;
#endif

            return const_cast<ClassType *>(static_cast<const ClassType *>(returnInstance));
        }

        template <typename ClassType>
        void setBorrowedInstance(ClassType *instance) {
            assert(!createPtr);
            assert(!returnInstance);
            assert(!managedInstance);
            assert(!destroyPtr);

#if JWUTIL_CONTEXT_ENABLE_STRUCT_GENERATION
            typeName = getTypeName<ClassType>();
            setBorrowedCount++;
#endif

            createPtr = &ClassEntry::createBorrowedInstanceError;
            returnInstance = static_cast<const void *>(instance);
            managedInstance = 0;
            isConst = std::is_const<ClassType>::value;
            destroyPtr = &ClassEntry::noop;
        }

        template <typename ClassType>
        ClassType *swapBorrowedInstance(ClassType *newInstance) {
            assert(createPtr == &ClassEntry::createBorrowedInstanceError);
            assert(returnInstance);
            assert(!managedInstance);
            assert(isConst == std::is_const<ClassType>::value);
            assert(destroyPtr == &ClassEntry::noop);

#if JWUTIL_CONTEXT_ENABLE_STRUCT_GENERATION
            swapBorrowedCount++;
#endif

            ClassType *res = getInstance<ClassType>();
            returnInstance = static_cast<const void *>(newInstance);
            return res;
        }

        template <typename ReturnType, typename ManagedType, bool contextConstructor>
        void prepareManagedInstance() {
            assert(!createPtr);
            assert(!returnInstance);
            assert(!managedInstance);
            assert(!destroyPtr);

#if JWUTIL_CONTEXT_ENABLE_STRUCT_GENERATION
            typeName = getTypeName<ReturnType>();
            prepareManagedCount++;
#endif

            createPtr = &ClassEntry::createStub<ReturnType, ManagedType, contextConstructor>;
            destroyPtr = &ClassEntry::destroyStub<ReturnType, ManagedType>;
        }

        void createManagedInstance(Context *context) {
            assert(createPtr);
            assert(!returnInstance);
            assert(!managedInstance);
            assert(destroyPtr);

#if JWUTIL_CONTEXT_ENABLE_STRUCT_GENERATION
            createManagedCount++;
#endif

            (this->*createPtr)(context);
            createPtr = 0;
        }

        void release() {
            (this->*destroyPtr)();
        }

#if JWUTIL_CONTEXT_ENABLE_STRUCT_GENERATION
        const char *typeName = 0;
        mutable unsigned int getCount = 0;
        mutable unsigned int setBorrowedCount = 0;
        mutable unsigned int swapBorrowedCount = 0;
        mutable unsigned int prepareManagedCount = 0;
        mutable unsigned int createManagedCount = 0;
#endif

    private:
        void (ClassEntry::*createPtr)(Context *context) = 0;
        const void *returnInstance = 0;
        const void *managedInstance = 0;
        bool isConst;
        void (ClassEntry::*destroyPtr)() = 0;

        template <typename ReturnType, typename ManagedType, bool contextConstructor>
        void createStub(Context *context) {
            if (JWUTIL_CONTEXT_ENABLE_DEBUG_INFO) {
                logInfo(context, "Context::createStub: ", getTypeName<ReturnType>(), ", ", getTypeName<ManagedType>());
            }
            assert(!managedInstance);
            assert(!returnInstance);
            createPtr = &ClassEntry::doubleCreateError;
            ManagedType *instance = construct<ManagedType, contextConstructor>(context);
            returnInstance = static_cast<const ReturnType *>(instance);
            managedInstance = static_cast<const void *>(instance);
            isConst = std::is_const<ManagedType>::value;
        }

        template <typename ManagedType, bool contextConstructor>
        static typename std::enable_if<contextConstructor, ManagedType *>::type construct(Context *context) {
            return new ManagedType(*static_cast<DerivedType *>(context));
        }

        template <typename ManagedType, bool contextConstructor>
        static typename std::enable_if<!contextConstructor, ManagedType *>::type construct(Context *context) {
            return new ManagedType();
        }

        template <typename ReturnType, typename ManagedType>
        void destroyStub() {
            if (JWUTIL_CONTEXT_ENABLE_DEBUG_INFO) {
                logInfo(0, "Context::destroyStub: ", getTypeName<ReturnType>(), ", ", getTypeName<ManagedType>());
            }
            delete static_cast<const ManagedType *>(managedInstance);
            returnInstance = 0;
            managedInstance = 0;
        }

        void createBorrowedInstanceError(Context *context) {
            if (JWUTIL_CONTEXT_ENABLE_DEBUG_INFO) {
                logInfo(context, "Context::createBorrowedInstanceError");
            }
            assert(false);
        }

        void doubleCreateError(Context *context) {
            if (JWUTIL_CONTEXT_ENABLE_DEBUG_INFO) {
                logInfo(context, "Context::doubleCreateError: This is probably because you have a circular dependency");
            }
            assert(false);
        }

        void noop() {}
    };

#if JWUTIL_CONTEXT_ENABLE_STRUCT_GENERATION
    std::string generateStruct() const {
        std::string className = getTypeName<DerivedType>();

        std::string res;
        res += "class " + className + " {\n";
        res += "public:\n";

        unsigned int varIndex = 0;
        typename std::vector<ClassEntry *>::const_iterator i = classOrder.cbegin();
        while (i != classOrder.cend()) {
            const ClassEntry &entry = **i;

            std::string indent = "    ";

            res += indent + entry.typeName + " *_" + std::to_string(varIndex) + ";\n";
            res += indent + "template <>\n";
            res += indent + entry.typeName + " *get<" + entry.typeName + ">() { return _" + std::to_string(varIndex) + "; }\n";

            varIndex++;
            i++;
        }

        res += "};\n";
        return res;
    }

    std::string getStructFilename() const {
        std::string className = getTypeName<DerivedType>();
        std::string::iterator i = className.begin();
        while (i != className.end()) {
            bool isLetterLc = *i >= 'a' && *i <= 'z';
            bool isLetterUc = *i >= 'A' && *i <= 'Z';
            bool isNumber = *i >= '0' && *i <= '9';
            bool isSymbol = *i == '_';
            if (!isLetterLc && !isLetterUc && !isNumber && !isSymbol) {
                *i = '_';
            }
            i++;
        }
        className += ".gen.h";
        return className;
    }
#endif

    template <typename... ArgTypes>
    static void logInfo(Context *context, ArgTypes... args) {
        std::string msg = toString<ArgTypes...>(args...);
#ifdef SPDLOG_VERSION
        if (context && context->isProvided<spdlog::logger>()) {
            context->get<spdlog::logger>().debug(msg);
            return;
        }
#endif
        std::cout << msg << std::endl;
    }

    template <typename... ArgTypes>
    static void logError(Context *context, ArgTypes... args) {
        std::string msg = toString<ArgTypes...>(args...);
#ifdef SPDLOG_VERSION
        if (context && context->isProvided<spdlog::logger>()) {
            context->get<spdlog::logger>().error(msg);
            return;
        }
#endif
        std::cerr << msg << std::endl;
    }

    template <typename FirstArgType, typename... RestArgTypes>
    static std::string toString(FirstArgType firstArg, RestArgTypes... restArgs) {
        return std::string(firstArg) + toString<RestArgTypes...>(restArgs...);
    }
    template <typename FirstArgType>
    static std::string toString(FirstArgType firstArg) {
        return std::string(firstArg);
    }

    template <typename Type>
    static const char *getTypeName() {
        static thread_local int status = 0;
        static thread_local const char *realname = abi::__cxa_demangle(typeid(Type).name(), 0, 0, &status);
        assert(status == 0);
        // TODO: Release allocated string
        return realname;
    }

    std::vector<ClassEntry *> classOrder;
    std::unordered_map<std::type_index, ClassEntry> classMap;
};

}

#endif // JWUTIL_CONTEXT_H
