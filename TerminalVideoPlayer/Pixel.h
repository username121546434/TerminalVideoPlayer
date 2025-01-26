#pragma once
#include <cstdint>
#include <opencv2/core/matx.hpp>

struct Pixel {
    Pixel() : r {0}, g {0}, b {0} {}
    Pixel(cv::Vec3b pixel);

    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct TerminalPixel {
    TerminalPixel() : top_pixel {}, bottom_pixel {} {}
    TerminalPixel(Pixel top, Pixel bottom) : top_pixel {top}, bottom_pixel {bottom} {}

    // top_pixel is the top half of the character block
    // this should be the background color
    Pixel top_pixel;
    // bottom_pixel is the bottom half of the character block
    // this should be the foreground color
    Pixel bottom_pixel;
};

