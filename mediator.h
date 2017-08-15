#ifndef JWUTIL_MEDIATOR_H
#define JWUTIL_MEDIATOR_H

#include <assert.h>
#include <type_traits>
#include <vector>
#include <algorithm>

#include "methodcallback.h"

namespace jw_util
{

class Mediator {
public:
    template <typename MessageType>
    class Listener {
    public:
        Listener(const jw_util::MethodCallback<MessageType> &callback_arg)
            : callback(callback_arg)
        {
            enable();
        }

        ~Listener() {
            disable();
        }

        void enable() {
            CallbackSet<typename std::decay<MessageType>::type>::add(callback);
        }

        void disable() {
            CallbackSet<typename std::decay<MessageType>::type>::remove(callback);
        }

    private:
        jw_util::MethodCallback<MessageType> callback;
    };

    template <typename MessageType>
    static void trigger(MessageType message) {
        CallbackSet<typename std::decay<MessageType>::type>::call(message);
    }

private:
    template <typename MessageType>
    class CallbackSet {
    public:
        static void add(const jw_util::MethodCallback<MessageType> &callback) {
            callbacks.push_back(callback);
        }

        static void remove(const jw_util::MethodCallback<MessageType> &callback) {
            typename std::vector<jw_util::MethodCallback<MessageType>>::reverse_iterator found
                = std::find(callbacks.rbegin(), callbacks.rend(), callback);
            assert(found != callbacks.rend());
            *found = callbacks.back();
            callbacks.pop_back();
        }

        static void call(const MessageType &message) {
            typename std::vector<jw_util::MethodCallback<MessageType>>::const_iterator i
                = callbacks.cbegin();
            while (i != callbacks.cend()) {
                // Can't use std::forward here because we're re-using the message
                i->call(message);
                i++;
            }
        }

    private:
        static std::vector<jw_util::MethodCallback<MessageType>> callbacks;
    };
};

template<typename MessageType>
std::vector<jw_util::MethodCallback<MessageType>> Mediator::CallbackSet<MessageType>::callbacks;

}

#endif // JWUTIL_MEDIATOR_H
