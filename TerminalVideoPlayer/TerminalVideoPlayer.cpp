#include <iostream>
#include <iomanip>
#include <string>
#include <locale>
#include <opencv2/videoio.hpp>
#include "get_terminal_size.h"
#include "utils.h"
#include <fmt/core.h>
#include <map>

constexpr const char *block = u8"\u2584"; // ? character
constexpr double threshold = 25.0;
constexpr double optimization_threshold = 0.40;

typedef std::vector<std::vector<Pixel>> Frame;

using namespace cv;

int divmod(int &i, int d) {
    int ret = i / d;
    i = i % d;
    return ret;
}

std::string format_seconds(int s) {
    int m {divmod(s, 60)};
    int h {divmod(m, 60)};
    
    std::stringstream formatted;
    formatted << std::setfill('0') << std::setw(2);
    if (h == 0)
        formatted << m << ":" << std::setfill('0') << std::setw(2) << s;
    else
        formatted << h << ":" << m << std::setfill('0') << std::setw(2) << ":" << std::setfill('0') << std::setw(2) << s;

    return formatted.str();
}

void display_status_bar(int curr_frame, int total_frames, int duration_seconds, int fps, double curr_fps, int width, int height) {
    int seconds_watched {curr_frame / fps};
    std::string result;
    set_cursor(0, 0, result);
    std::cout << result;
    std::cout << "\033[0mFrame " << curr_frame << "/" << total_frames << " " << width << "x" << height << " "
              << format_seconds(seconds_watched) << "/" << format_seconds(duration_seconds) << " "
              << std::setprecision(2) << std::fixed << curr_fps << "fps"
              << '\n';
}

void update_pixel(Pixel new_pixel, size_t x, size_t y, std::string &result, std::vector<std::vector<Pixel>> &currently_displayed);
void display_entire_frame(std::string &result, const std::vector<std::vector<Pixel>> &currently_displayed);
void process_new_frame(const cv::Mat &frame, std::string &result, std::vector<std::vector<Pixel>> &currently_displayed);

void init_currently_displayed(const cv::Mat &start_frame, std::vector<std::vector<Pixel>> &currently_displayed) {
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

void process_new_frame(const cv::Mat &frame, std::string &result, std::vector<std::vector<Pixel>> &currently_displayed) {
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
        display_entire_frame(result, currently_displayed);
    } else
        for (int row = 0; row < frame.rows; ++row) {
            for (int col = 0; col < frame.cols; ++col) {
                Pixel new_p {frame.at<cv::Vec3b>(row, col)};

                if (!pixel_diffs[row][col])
                    continue;

                update_pixel(new_p, col, row, result, currently_displayed);
            }
        }
}

void update_pixel(Pixel new_pixel, size_t x, size_t y, std::string &result, std::vector<std::vector<Pixel>> &currently_displayed) {
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

void display_entire_frame(std::string &result, const std::vector<std::vector<Pixel>> &currently_displayed) {
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

int main(int argc, char *argv[]) {
    std::string file;
    std::setlocale(LC_ALL, "");
    if (argc > 1)
        file = argv[1];
    else {
        std::cout << "Enter a video file: ";
        std::getline(std::cin, file);
    }
    VideoCapture video {file, 0};
    if (!video.isOpened()) {
        std::cerr << "Error opening file " << file << std::endl;
        return 1;
    }

    double total_frames {video.get(VideoCaptureProperties::CAP_PROP_FRAME_COUNT)};
    double fps {video.get(VideoCaptureProperties::CAP_PROP_FPS)};
    double duration_seconds {total_frames / fps};
    double curr_fps {};
    long long target_frame_time {static_cast<long long>(1.0 / static_cast<long long>(fps + 1) * 1000)};
    std::pair last_size {0, 0};
    int frames_to_drop {0};
    
    Frame currently_displayed;
    std::map<int, Frame> frame_list;
    std::map<int, cv::Mat> frame_data;
    std::map<int, std::string> frame_buffer;

    auto [width, height] = get_terminal_size();
    height = height * 2 - 4;

    for (int curr_frame = 1; curr_frame < total_frames; ++curr_frame) {
        auto startTime = std::chrono::high_resolution_clock::now();

        Mat data;
        video.read(data);

        data = resize_mat(data, height, width);
        frame_data[curr_frame] = data;

        if (frames_to_drop > 0) {
            frames_to_drop--;
            continue;
        }
        std::string to_display;
        to_display.reserve(width * height * 3);
        if (curr_frame == 1) {
            init_currently_displayed(data, currently_displayed);
            display_entire_frame(to_display, currently_displayed);
        } else {
            process_new_frame(data, to_display, currently_displayed);
        }
        frame_list[curr_frame] = currently_displayed;
        frame_buffer[curr_frame] = to_display;
        const Frame &frame {currently_displayed};

        display_status_bar(curr_frame, total_frames, duration_seconds, fps, curr_fps, frame[0].size(), frame.size());
        fmt::print(stdout, to_display);

        auto endTime = std::chrono::high_resolution_clock::now();
        auto elapsedTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        int sleepTimeMs = target_frame_time - static_cast<int>(elapsedTimeMs);
        if (sleepTimeMs > 0) {
            curr_fps = fps;
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMs));
        } else {
            curr_fps = 1.0 / (elapsedTimeMs / 1000.0);
            frames_to_drop = fps / curr_fps;
        }

    }

    video.release();
    fmt::print("\033[0m"); // resets terminal color so that the user can continue with the same window
}
