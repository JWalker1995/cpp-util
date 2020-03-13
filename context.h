#ifndef JWUTIL_CONTEXT_H
#define JWUTIL_CONTEXT_H

#define JWUTIL_CONTEXT_ENABLE_STRUCT_GENERATION 0

#if JWUTIL_CONTEXT_ENABLE_STRUCT_GENERATION
#include <fstream>
#endif

#include <assert.h>
#include <typeindex>
#include <vector>
#include <string>
#include <unordered_map>
#include <cxxabi.h>

namespace jw_util {

template <typename DerivedType>
class Context {
public:
    enum class LogLevel {
        Trace,
        Info,
        Warning,
        Error,
    };

    Context(float loadFactor = 0.1f) {
        // Trade memory for speed
        classMap.max_load_factor(loadFactor);
    }

    Context(const Context& other) = delete;
    Context &operator=(const Context &other) = delete;

    Context(Context&&) = delete;
    Context& operator=(Context&&) = delete;


    template <typename InterfaceType>
    void provideInstance(InterfaceType *instance) {
        assert(instance);
        emitLog(LogLevel::Info, getTypeName<DerivedType>() + "::provideInstance<" + getTypeName<InterfaceType>() + ">()");

        ClassEntry entry;
        entry.template setBorrowedInstance<InterfaceType>(instance);
        auto inserted = classMap.emplace(std::type_index(typeid(InterfaceType)), entry);
        assert(inserted.second);
    }

    template <typename InterfaceType>
    InterfaceType *swapInstance(InterfaceType *newInstance) {
        assert(newInstance);
        emitLog(LogLevel::Trace, getTypeName<DerivedType>() + "::swapInstance<" + getTypeName<InterfaceType>() + ">()");

        auto found = classMap.find(std::type_index(typeid(InterfaceType)));
        assert(found != classMap.end());
        return found->second.template swapBorrowedInstance<InterfaceType>(newInstance);
    }

    template <typename InterfaceType>
    InterfaceType *removeInstance() {
        emitLog(LogLevel::Info, getTypeName<DerivedType>() + "::removeInstance<" + getTypeName<InterfaceType>() + ">()");

        auto found = classMap.find(std::type_index(typeid(InterfaceType)));
        assert(found != classMap.end());
        InterfaceType *res = found->second.template swapBorrowedInstance<InterfaceType>(0);
        classMap.erase(found);
        return res;
    }


    template <typename InterfaceType>
    bool has() {
        // Not even trace level
        // emitLog(LogLevel::Trace, getTypeName<DerivedType>() + "::has<" + getTypeName<InterfaceType>() + ">()");

        return classMap.find(std::type_index(typeid(InterfaceType))) != classMap.end();
    }

    template <typename InterfaceType, typename ImplementationType = InterfaceType, typename... ArgTypes>
    InterfaceType &construct(ArgTypes... args) {
        emitLog(LogLevel::Trace, getTypeName<DerivedType>() + "::construct<" + getTypeName<InterfaceType>() + ", " + getTypeName<ImplementationType>() + ">()");

        auto inserted = classMap.emplace(std::type_index(typeid(InterfaceType)), ClassEntry());
        assert(inserted.second);

        inserted.first->second.template createManagedInstance<InterfaceType, ImplementationType, ArgTypes...>(this, std::forward<ArgTypes>(args)...);
        classOrder.push_back(&inserted.first->second);

        return *inserted.first->second.template getInstance<InterfaceType>();
    }

    template <typename InterfaceType, typename ImplementationType = InterfaceType, typename... ArgTypes>
    InterfaceType &get(ArgTypes... args) {
        // Not even trace level
        // emitLog(LogLevel::Trace, getTypeName<DerivedType>() + "::get<" + getTypeName<InterfaceType>() + ">()");

        return internalGet<InterfaceType, ImplementationType, ArgTypes...>(0, std::forward<ArgTypes>(args)...);
    }

    unsigned int getManagedTypeCount() const {
        return classOrder.size();
    }

    unsigned int getTotalTypeCount() const {
        return classMap.size();
    }

    std::vector<std::string> getTypeNames() const {
        std::vector<std::string> res;
        res.reserve(classMap.size());
        typename std::unordered_map<std::type_index, ClassEntry>::const_iterator i = classMap.cbegin();
        while (i != classMap.cend()) {
            res.push_back(makeTypeName(i->first));
            i++;
        }
        return res;
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
            // std::cout << getTypeName<DerivedType>() + "::createSomething: rand(" << classMap.size() << ") -> " << index << std::endl;
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

    void reset() {
        runDestructors();
        classOrder.clear();
        classMap.clear();
    }

    ~Context() {
        runDestructors();
    }

private:
    class ClassEntry {
    public:
        bool hasInstance() const {
            return returnInstance;
        }

        template <typename ClassType>
        ClassType *getInstance() const {
            assert(returnInstance && "If this fails, you probably have a circular dependency");
            assert(std::is_const<ClassType>::value || !isConst);

#if JWUTIL_CONTEXT_ENABLE_STRUCT_GENERATION
            getCount++;
#endif

            return const_cast<ClassType *>(static_cast<const ClassType *>(returnInstance));
        }

        template <typename ClassType>
        void setBorrowedInstance(ClassType *instance) {
            assert(!returnInstance);
            assert(!managedInstance);
            assert(!destroyPtr);

#if JWUTIL_CONTEXT_ENABLE_STRUCT_GENERATION
            typeName = getTypeName<ClassType>();
            setBorrowedCount++;
#endif

            returnInstance = static_cast<const void *>(instance);
            managedInstance = 0;
            isConst = std::is_const<ClassType>::value;
            destroyPtr = &ClassEntry::noop;
        }

        template <typename ClassType>
        ClassType *swapBorrowedInstance(ClassType *newInstance) {
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

        template <typename ReturnType, typename ManagedType, typename... ArgTypes>
        void createManagedInstance(Context *context, ArgTypes... args) {
            assert(!returnInstance);
            assert(!managedInstance);
            assert(!destroyPtr);

#if JWUTIL_CONTEXT_ENABLE_STRUCT_GENERATION
            createManagedCount++;
#endif

            destroyPtr = &ClassEntry::destroyStub<ReturnType, ManagedType>;
            ManagedType *instance = new ManagedType(*static_cast<DerivedType *>(context), std::forward<ArgTypes>(args)...);
            returnInstance = static_cast<const ReturnType *>(instance);
            managedInstance = static_cast<const void *>(instance);
            isConst = std::is_const<ManagedType>::value;
        }

        void release(Context *context) {
            (this->*destroyPtr)(context);
            destroyPtr = 0;
            returnInstance = 0;
        }

#if JWUTIL_CONTEXT_ENABLE_STRUCT_GENERATION
        std::string typeName;
        mutable unsigned int getCount = 0;
        mutable unsigned int setBorrowedCount = 0;
        mutable unsigned int swapBorrowedCount = 0;
        mutable unsigned int prepareManagedCount = 0;
        mutable unsigned int createManagedCount = 0;
#endif

    private:
        void (ClassEntry::*destroyPtr)(Context *context) = 0;
        const void *returnInstance = 0;
        const void *managedInstance = 0;
        bool isConst;

        template <typename ReturnType, typename ManagedType>
        void destroyStub(Context *context) {
            context->emitLog(LogLevel::Info, getTypeName<DerivedType>() + "::destroyStub<" + getTypeName<ReturnType>() + ", " + getTypeName<ManagedType>() + ">()");

            delete static_cast<const ManagedType *>(managedInstance);
            returnInstance = 0;
            managedInstance = 0;
        }

        void noop(Context *context) {
            (void) context;
        }
    };

    template <typename InterfaceType, typename ImplementationType, typename... ArgTypes, typename = decltype(ImplementationType(std::declval<DerivedType &>(), std::declval<ArgTypes>()...))>
    InterfaceType &internalGet(int, ArgTypes... args) {
        auto inserted = classMap.emplace(std::type_index(typeid(InterfaceType)), ClassEntry());
        if (inserted.second) {
            inserted.first->second.template createManagedInstance<InterfaceType, ImplementationType, ArgTypes...>(this, std::forward<ArgTypes>(args)...);
            classOrder.push_back(&inserted.first->second);
        }

        return *inserted.first->second.template getInstance<InterfaceType>();
    }

    template <typename InterfaceType, typename ImplementationType>
    InterfaceType &internalGet(...) {
        static_assert(std::is_same<InterfaceType, ImplementationType>::value, "Supplied an ImplementationType to Context::get, but it is not constructible with those arguments");

        auto found = classMap.find(std::type_index(typeid(InterfaceType)));
        if (found == classMap.cend()) {
            emitLog(LogLevel::Error, getTypeName<DerivedType>() + "::get: Cannot find non-constructible type " + getTypeName<InterfaceType>());
            assert(false);
        }

        return *found->second.template getInstance<InterfaceType>();
    }

    void runDestructors() {
#if JWUTIL_CONTEXT_ENABLE_STRUCT_GENERATION
        std::ofstream file;
        file.open(getStructFilename());
        file << generateStruct();
        file.close();
#endif

        typename std::vector<ClassEntry *>::reverse_iterator i = classOrder.rbegin();
        while (i != classOrder.rend()) {
            ClassEntry &entry = **i;
            entry.release(this);
            i++;
        }
    }

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

    void emitLog(LogLevel level, const std::string &msg) {
        static_cast<DerivedType *>(this)->log(level, msg);
    }

    static std::string makeTypeName(std::type_index typeId) {
        int status = 0;
        std::size_t length;
        char *realname = abi::__cxa_demangle(typeId.name(), 0, &length, &status);
        assert(status == 0);

        std::string str(realname, length - 1);

        std::free(realname);

        return str;
    }

    template <typename Type>
    static std::string getTypeName() {
        static thread_local std::string str = makeTypeName(typeid(Type));
        return str;
    }

    std::vector<ClassEntry *> classOrder;
    std::unordered_map<std::type_index, ClassEntry> classMap;
};

}

#endif // JWUTIL_CONTEXT_H
