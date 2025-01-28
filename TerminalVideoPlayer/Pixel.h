#pragma once
#include <cstdint>

struct Pixel {
    Pixel() : r {0}, g {0}, b {0} {}
    Pixel(uint8_t r, uint8_t g, uint8_t b) : r {r}, g {g}, b {b} {}

    uint8_t r;
    uint8_t g;
    uint8_t b;

    bool operator==(Pixel other) const {
        return r == other.r && g == other.g && b == other.b;
    }
    bool operator!=(Pixel other) const {
        return !(*this == other);
    }
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

