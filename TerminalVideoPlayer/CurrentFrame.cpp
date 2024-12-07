#include "CurrentFrame.h"
#include "utils.h"
#include <iostream>
#include <fmt/core.h>
#include <fstream>

constexpr double threshold = 20.0;
constexpr double optimization_threshold = 0.40;

CurrentFrame::CurrentFrame(const cv::Mat &start_frame) : currently_displayed {} {
    init_currently_displayed(start_frame);
}

void CurrentFrame::init_currently_displayed(const cv::Mat &start_frame) {
    currently_displayed.resize(start_frame.rows);
    for (int row = 0; row < start_frame.rows; ++row) {
        auto &curr_row {currently_displayed[row]};
        curr_row.reserve(start_frame.cols);

        for (int col = 0; col < start_frame.cols; ++col) {
            cv::Vec3b pixel = start_frame.at<cv::Vec3b>(row, col);
            curr_row.emplace_back(pixel);
        }
    }
}

void CurrentFrame::process_new_frame(const cv::Mat &frame, std::string &result) {
    int pixels {frame.rows * frame.cols};
    size_t diff {};
    std::vector<std::vector<Pixel>> potential_new_curr_displayed {currently_displayed};
    std::vector<std::vector<bool>> pixel_diffs;
    pixel_diffs.reserve(potential_new_curr_displayed.size());

    for (int row = 0; row < frame.rows; ++row) {
        auto &curr_row {currently_displayed[row]};
        std::vector<bool> row_pixel_diffs;
        row_pixel_diffs.resize(frame.cols);
        for (int col = 0; col < frame.cols; ++col) {
            Pixel p {curr_row[col]};
            Pixel new_p {frame.at<cv::Vec3b>(row, col)};

            potential_new_curr_displayed[row][col] = new_p;

            if (distance(p, new_p) <= threshold) {
                continue;
            }
            
            row_pixel_diffs[col] = true;
            diff++;
        }
        pixel_diffs.push_back(row_pixel_diffs);
    }

    double ratio {diff / static_cast<double>(pixels)};
    if (ratio > optimization_threshold) {
        currently_displayed = potential_new_curr_displayed;
        display_entire_frame(result);
    } else
        for (int row = 0; row < frame.rows; ++row) {
            for (int col = 0; col < frame.cols; ++col) {
                Pixel new_p {frame.at<cv::Vec3b>(row, col)};

                if (!pixel_diffs[row][col])
                    continue;

                update_pixel(new_p, col, row, result);
            }
        }
}

void CurrentFrame::update_pixel(Pixel new_pixel, size_t x, size_t y, std::string &result) {
    currently_displayed[y][x] = new_pixel;

    auto new_y {y / 2 + 2};
    auto new_x {x + 1};
    set_cursor(x, new_y, result);
    bool is_odd_pixel {static_cast<bool>(y % 2)};
    set_color(new_pixel, !is_odd_pixel, result);
    if (is_odd_pixel)
        set_color(currently_displayed[y - 1][x], true, result);
    else if (y + 1 != currently_displayed.size())
        set_color(currently_displayed[y + 1][x], false, result);
    result += block;

    if (x == currently_displayed[y].size() - 1) {
        result.push_back(esc);
        result += "[0m";
    }
}

void CurrentFrame::display_entire_frame(std::string &result) {
    int y {0};
    for (int y {0}; y < currently_displayed.size(); y += 2) {
        const auto &top_row {currently_displayed.at(y)};
        const auto &bottom_row {currently_displayed.at(y + 1)};
        for (int x {0}; x < top_row.size(); x++) {
            Pixel top_pixel {top_row.at(x)};
            Pixel bottom_pixel {bottom_row.at(x)};
            set_color(top_pixel, true, result);
            set_color(bottom_pixel, false, result);
            result += block;
            if (x == top_row.size() - 1)
                result += "\033[0m\n";
        }
    }
}
