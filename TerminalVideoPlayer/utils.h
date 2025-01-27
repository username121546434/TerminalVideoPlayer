#pragma once
#include "Pixel.h"
#include <opencv2/core/mat.hpp>
#include <filesystem>
#include <random>
#include <string>

const char esc = '\x1B';

double distance(Pixel p1, Pixel p2);

cv::Mat resize_mat(const cv::Mat &frame, size_t height, size_t width);

std::filesystem::path create_temp_directory(unsigned long long max_tries = 100);

void set_cursor(size_t x, size_t y, std::string &result);
void set_color(Pixel p, bool bg, std::string &result);
void clear_screen();
