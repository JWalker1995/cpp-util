#ifndef JWUTIL_BASEEXCEPTION_H
#define JWUTIL_BASEEXCEPTION_H

#include <exception>
#include <string>

#include "jw_util/to_string.h"

namespace jw_util {

class BaseException : public std::exception {
public:
    virtual const char *what() const noexcept {
        return str.c_str();
    }

protected:
    template <typename... ArgTypes>
    BaseException(ArgTypes &... args)
        : str(jw_util::to_string(args...))
    {}

    std::string str;
};

}

#endif // JWUTIL_BASEEXCEPTION_H
