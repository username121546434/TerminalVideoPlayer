#pragma once
#include <cstdint>
#include <opencv2/core/matx.hpp>

struct Pixel {
    Pixel(cv::Vec3b pixel);
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

