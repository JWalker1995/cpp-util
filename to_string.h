#ifndef TO_STRING_H
#define TO_STRING_H

#include <string>

namespace jw_util {

template<typename ArgType>
inline static std::string to_string(const ArgType &arg) {
    return typeid(ArgType).name();
}

inline static std::string to_string(int arg) {return std::to_string(arg);}
inline static std::string to_string(long arg) {return std::to_string(arg);}
inline static std::string to_string(long long arg) {return std::to_string(arg);}
inline static std::string to_string(unsigned arg) {return std::to_string(arg);}
inline static std::string to_string(unsigned long arg) {return std::to_string(arg);}
inline static std::string to_string(unsigned long long arg) {return std::to_string(arg);}
inline static std::string to_string(float arg) {return std::to_string(arg);}
inline static std::string to_string(double arg) {return std::to_string(arg);}
inline static std::string to_string(long double arg) {return std::to_string(arg);}
inline static std::string to_string(const char *arg) {return arg;}
inline static std::string to_string(const std::string &arg) {return arg;}

inline static std::string to_string(char *const *arg) {
    std::string res;

    if (*arg == 0) {return res;}
    while (true) {
        res += *arg;
        arg++;
        if (*arg == 0) {return res;}
        res += ", ";
    }
}
inline static std::string to_string(char **arg) {
    return to_string(const_cast<char *const *>(arg));
}

}
#endif // TO_STRING_H
