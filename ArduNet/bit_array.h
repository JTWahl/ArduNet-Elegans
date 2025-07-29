#ifndef BIT_ARRAY_H
#define BIT_ARRAY_H

#include <stdint.h>

template <uint16_t Length>
class BitArray {

    public:
    static const uint16_t length = Length;

    private:
    static constexpr uint8_t size = (length + 7) / 8 ; // size in bytes
    uint8_t arr[size] = {0};

    // holds reference to byte in array and specific bit so bits can be set with subscript operator
    struct bit_proxy {
        uint8_t* byte;
        uint8_t bit;
        void operator=(bool val) {
            *byte = (*byte & ~(1 << bit)) | (val << bit); // change bit in referenced byte when setting
        }
        operator int() {
            return (*byte & (1 << bit)) >> bit; // get the bit on its own for standard use
        }
    };

    public:
    // when subscripting, quietly return a proxy that references the byte and remembers the relevant bit
    bit_proxy operator[](uint16_t idx) {
        bit_proxy b = { arr + (idx / 8), static_cast<uint8_t>(idx % 8) };
        return b;
    }

};

#endif
