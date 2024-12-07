#pragma once
#include <vector>
#include <opencv2/videoio.hpp>
#include "Pixel.h"

constexpr const char *block = u8"\u2584"; // ? character

class CurrentFrame {
    std::vector<std::vector<Pixel>> currently_displayed;
public:
    CurrentFrame() = default;
    CurrentFrame(const cv::Mat &start_frame);

    void init_currently_displayed(const cv::Mat &start_frame);

    void process_new_frame(const cv::Mat &frame, std::string &result);
    void update_pixel(Pixel new_pixel, size_t x, size_t y, std::string &result);
    void display_entire_frame(std::string &result);
};

