#include "utils.h"
#include <math.h>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <stdlib.h>
#include <fmt/core.h>
#include <algorithm>
#include <framework.h>

double distance(Pixel p1, Pixel p2) {
    return std::sqrt(std::pow(p1.r - p2.r, 2) + std::pow(p1.g - p2.g, 2) + std::pow(p1.b - p2.b, 2));
}

cv::Mat resize_mat(const cv::Mat &frame, size_t height, size_t width) {
    double scale {std::min(static_cast<double>(width) / frame.cols, static_cast<double>(height) / frame.rows)};
    cv::Size size {static_cast<int>(frame.cols * scale), static_cast<int>(frame.rows * scale)};
    cv::Mat m;
    cv::resize(frame, m, size, 0, 0, cv::INTER_AREA);
    return m;
    //return PillowResize::resize(frame, size, PillowResize::INTERPOLATION_BICUBIC);
}

void set_cursor(size_t x, size_t y, std::string &result) {
    fmt::format_to(std::back_inserter(result), "{}[{};{}H", esc, y, x + 1);
}

void set_color(Pixel p, bool bg, std::string &result) {
    //std::cout << "\033[" << (bg ? 48 : 38) << ";2;" << p.r << ';' << p.g << ';' << p.b << 'm';
    fmt::format_to(std::back_inserter(result), "{}[{};2;{};{};{}m", esc, bg ? 48 : 38, p.r, p.g, p.b);
}

void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}
