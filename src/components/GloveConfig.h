#ifndef UDCAP_GLOVE_CONFIG_H
#define UDCAP_GLOVE_CONFIG_H

#include <string>

inline std::string gloveConfigPrefix(const std::string &serial) {
    constexpr char digits[] = "0123456789ABCDEF";
    std::string encoded;
    encoded.reserve(serial.size() * 2);
    for (unsigned char c : serial) {
        encoded += digits[c >> 4];
        encoded += digits[c & 15];
    }
    return "/enterprise/gloves/" + encoded;
}

#endif
