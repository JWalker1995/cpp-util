#ifndef TO_STRING_H
#define TO_STRING_H

#include <string>

namespace jw_util {

template<typename ArgType>
static std::string to_string(const ArgType &arg) {
    return std::to_string(arg);
}
static std::string to_string(const char *arg) {
    return arg;
}
static std::string to_string(const std::string &arg) {
    return arg;
}
static std::string to_string(char *const *arg) {
    std::string res;

    if (*arg == 0) {return res;}
    while (true) {
        res += *arg;
        arg++;
        if (*arg == 0) {return res;}
        res += ", ";
    }
}
static std::string to_string(char **arg) {
    return to_string(const_cast<char *const *>(arg));
}

}
#endif // TO_STRING_H
