#ifndef BASE62_HPP
#define BASE62_HPP

#include <string>
#include <algorithm>
#include <cstdint>

static const char ALPHABET[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

struct Base62 {
    static std::string Encode(uint64_t id) {
        if (id == 0) return "0";

        std::string code;
        while (id > 0) {
            code += ALPHABET[id % 62];
            id /= 62;
        }
        std::reverse(code.begin(), code.end());
        return code;
    }

    static uint64_t Decode(const std::string& code) {
        if (code.empty()) return 0;

        uint64_t id = 0;
        for (char ch : code) {
            uint64_t val;
            if (ch >= '0' && ch <= '9')           val = ch - '0';
            else if (ch >= 'A' && ch <= 'Z')      val = ch - 'A' + 10;
            else if (ch >= 'a' && ch <= 'z')      val = ch - 'a' + 36;
            else                                  return 0;
            id = id * 62 + val;
        }
        return id;
    }
};

#endif
