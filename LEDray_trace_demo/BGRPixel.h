#pragma once
#include <cstdint>
#pragma pack(push, 1) //Ensure no padding between fields

//Just what is says, a pixel stored in form of BGR (blue, green, red) with 8 bits per channel. This is the format used by Windows bitmaps, and is also the format used by OpenCV when reading/writing images, so it is a convenient format for storing pixel data in this project. The struct is packed to ensure that there are no padding bytes between the fields, which allows for more raw usage, efficient storage, and faster access when reading/writing pixel data.
struct BGRPixel {
    uint8_t b = 0;
    uint8_t g = 0;
    uint8_t r = 0;
};

#pragma pack(pop)
