#include <iostream>
#include <assert.h>
#include <typeindex>
#include <unordered_map>
#include <random>

#define DEBUG_CONTEXT 1

class Context {
public:
    template <typename ClassType>
    void registerClass() {
        if (DEBUG_CONTEXT) {
            std::cout << "Context::registerClass: " << typeid(ClassType).name() << std::endl;
        }
        ClassEntry entry;
        entry.prepareManagedInstance<ClassType>();
        auto inserted = classes.emplace(std::type_index(typeid(ClassType)), entry);
        assert(inserted.second);
    }


    void createSomething() {
        assert(!classes.empty());

        // Any iteration over a std::unordered_map will be in an implementation-dependent order,
        // So we explicitly randomize it here so we don't get weird edge cases.
        static std::random_device randomDevice;
        static std::mt19937 randomEngine(randomDevice());
        unsigned int index = std::uniform_int_distribution<unsigned int>(0, classes.size() - 1)(randomEngine);
        if (DEBUG_CONTEXT) {
            std::cout << "Context::createSomething: rand(" << classes.size() << ") -> " << index << std::endl;
        }

        auto offset = std::next(classes.begin(), index);
        auto i = offset;
        do {
            if (!i->second.hasInstance()) {
                i->second.createManagedInstance(*this);
                return;
            }

            i++;
            if (i == classes.end()) {
                i = classes.begin();
            }
        } while (i != offset);

        // If we get here, there is a circular dependency
        assert(false);
    }

private:
    class ClassEntry {
    public:
        ~ClassEntry() {
            if (destroyPtr) {
                (this->*destroyPtr)();
            }
        }

        bool hasInstance() const {
            return instance;
        }

        template <typename ClassType>
        ClassType *getInstance() const {
            return static_cast<ClassType *>(instance);
        }

        template <typename ClassType>
        void prepareManagedInstance() {
            assert(!instance);
            createPtr = &ClassEntry::createStub<ClassType>;
            destroyPtr = &ClassEntry::destroyStub<ClassType>;
        }

        void createManagedInstance(Context &context) {
            assert(!instance);
            (this->*createPtr)(context);
        }

        template <typename ClassType>
        void setBorrowedInstance(ClassType *newInstance) {
            assert(!instance);
            instance = newInstance;
        }

    private:
        void (ClassEntry::*createPtr)(Context &context) = 0;
        void *instance = 0;
        void (ClassEntry::*destroyPtr)() = 0;

        template <typename ClassType>
        void createStub(Context &context) {
            if (DEBUG_CONTEXT) {
                std::cout << "Context::createStub: " << typeid(ClassType).name() << std::endl;
            }
            assert(!instance);
            instance = new ClassType(context);
        }

        template <typename ClassType>
        void destroyStub() {
            if (DEBUG_CONTEXT) {
                std::cout << "Context::destroyStub: " << typeid(ClassType).name() << std::endl;
            }
            delete static_cast<ClassType *>(instance);
            instance = 0;
        }
    };
};

class ContextHandle {
public:
    template <typename ClassType>
    void insertInstance(ClassType &instance) {
        if (DEBUG_CONTEXT) {
            std::cout << "Context::insertInstance: " << typeid(ClassType).name() << std::endl;
        }
        ClassEntry entry;
        entry.setBorrowedInstance<ClassType>(&instance);
        auto inserted = classes.emplace(std::type_index(typeid(ClassType)), entry);
        assert(inserted.second);
    }

    template <typename ClassType>
    ClassType &get() {
        if (DEBUG_CONTEXT) {
            std::cout << "Context::get: " << typeid(ClassType).name() << std::endl;
        }
        auto found = classes.find(std::type_index(typeid(ClassType)));
        if (found == classes.end()) {
            //assert(false);

            // Maybe the type is registered/inserted by a class that hasn't been instantiated yet
            createSomething();
            return get<ClassType>();
        }
        if (!found->second.hasInstance()) {
            found->second.createManagedInstance(*this);
        }
        return *found->second.getInstance<ClassType>();
    }

private:
    std::deque<ClassEntry> classEntries;
    std::unordered_map<std::type_index, ClassEntry *> classMap;
};

// jw_util::Context -> PreparedContext

class Bla {
public:
    Bla(Context &context) {}
};
class Ghi {
public:
    Ghi(Context &context) {
        context.registerClass<Bla>();
    }
};

class Abc {
public:
    Abc(Context &context) {
        context.registerClass<Ghi>();
    }
};
class Def {
public:
    Def(Context &context) {
    }
};

int main() {
    Context context;
    context.registerClass<Abc>();
    Def def(context);
    context.insertInstance(def);
    Bla &bla = context.get<Bla>();
    return 0;
}
