#pragma once

#include <cstdint>
#include <limits.h>
#include <type_traits>
#include <assert.h>

namespace jw_util {

template <unsigned int size>
class Bitset {
    typedef std::conditional<size <= 8,
        std::uint_fast8_t,
        std::conditional<size <= 16,
            std::uint_fast16_t,
            std::conditional<size <= 32,
                std::uint_fast32_t,
                std::uint_fast64_t
        >>> WordType;

    static constexpr unsigned int wordBits = sizeof(WordType) * CHAR_BIT;
    static constexpr unsigned int numWords = (size + wordBits - 1) / wordBits;

    static_assert((wordBits & (wordBits - 1)) == 0, "wordBits must be a power of 2");

public:
    class ValueIterator {
    public:
        ValueIterator(Bitset &bitset)
            : bitset(bitset)
        {
            findOne();
        }

        bool has() const {
            return curValue < size;
        }

        unsigned int get() const {
            return curValue;
        }

        void advance() {
            curValue++;
            findOne();
        }

        void flip() {
            bitset.words[curValue / wordBits] ^= static_cast<WordType>(1) << (curValue % wordBits);
        }

    private:
        Bitset &bitset;
        unsigned int curValue = 0;

        void findOne() {
            while (curValue < size) {
                WordType rest = bitset.words[curValue / wordBits] >> (curValue % wordBits);
                if (rest) {
                    curValue += getLsbIndex(rest);
                    break;
                } else {
                    curValue &= ~(wordBits - 1);
                    curValue += wordBits;

                    assert(curValue % wordBits == 0);
                }
            }
        }
    };

    template <bool value>
    void fill() {
        for (unsigned int i = 0; i < numWords; i++) {
            words[i] = value ? ~static_cast<WordType>(0) : 0;
        }

        if (value) {
            words[numWords - 1] &= (static_cast<WordType>(1) << (size % wordBits)) - 1;
        }
    }

    unsigned int getCount() const {
        unsigned int count = 0;

        for (unsigned int i = 0; i < numWords; i++) {
            count += getPopcount(words[i]);
        }

        return count;
    }

    unsigned int getFirstIndex() const {
        for (unsigned int i = 0; i < numWords; i++) {
            if (words[i]) {
                return getLsbIndex(words[i]) + i * wordBits;
            }
        }

#ifndef NDEBUG
        assert(false);
#else
        __builtin_unreachable();
#endif
    }

private:
    WordType words[numWords];

    static unsigned int getLsbIndex(unsigned int word) { return __builtin_ctz(word); }
    static unsigned int getLsbIndex(unsigned long word) { return __builtin_ctzl(word); }
    static unsigned int getLsbIndex(unsigned long long word) { return __builtin_ctzll(word); }

    static unsigned int getPopcount(unsigned int word) { return __builtin_popcount(word); }
    static unsigned int getPopcount(unsigned long word) { return __builtin_popcountl(word); }
    static unsigned int getPopcount(unsigned long long word) { return __builtin_popcountll(word); }
};

}
