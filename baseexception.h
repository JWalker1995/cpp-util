#ifndef JWUTIL_BASEEXCEPTION_H
#define JWUTIL_BASEEXCEPTION_H

#include <exception>
#include <string>

namespace jw_util {

class BaseException : public std::exception {
public:
    virtual const char *what() const noexcept {
        return str.c_str();
    }

protected:
    BaseException(const std::string &msg)
        : str(msg)
    {}

    std::string str;
};

}

#endif // JWUTIL_BASEEXCEPTION_H
