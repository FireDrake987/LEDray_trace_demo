#pragma once
#include <cstdint>
#pragma pack(push, 1) //Ensure no padding between fields

struct BGRPixel {
    uint8_t b = 0;
    uint8_t g = 0;
    uint8_t r = 0;
};

#pragma pack(pop)
