#include <iostream>
#include <iomanip>
#include <string>
#include <locale>
#include <opencv2/videoio.hpp>
#include "get_terminal_size.h"
#include "utils.h"
#include <fmt/core.h>

// experimental optimization, enable or disable if you wish
#define CURR_FRAME

#ifdef CURR_FRAME
#include "CurrentFrame.h"
#else
constexpr const char *block = u8"\u2584"; // ? character
#endif

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

void display_status_bar(int curr_frame, int total_frames, int duration_seconds, int fps, int width, int height) {
    int seconds_watched {curr_frame / fps};
    std::string result;
    set_cursor(0, 0, result);
    std::cout << result;
    std::cout << "\033[0mFrame " << curr_frame << "/" << total_frames << " " << width << "x" << height << " "
              << format_seconds(seconds_watched) << "/" << format_seconds(duration_seconds) << " "
              << '\n';
}

void draw_frame(const Mat &frame) {
    std::string to_print;
    to_print.reserve(frame.cols * frame.rows * 3);
    for (int y {0}; y < frame.rows; y += 2) {
        for (int x {0}; x < frame.cols; x++) {
            Pixel top_pixel {frame.at<cv::Vec3b>(y, x)};
            Pixel bottom_pixel {frame.at<cv::Vec3b>(y + 1, x)};
            set_color(top_pixel, true, to_print);
            set_color(bottom_pixel, false, to_print);
            to_print += block;
            if (x == frame.cols - 1)
                to_print += "\033[0m\n";
        }
    }
    fmt::print(to_print);
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
    long long target_frame_time {1 / static_cast<long long>(fps + 1) * 1000};

#ifdef CURR_FRAME
    CurrentFrame frame_drawer;
#endif

    for (int curr_frame {1}; curr_frame < total_frames; curr_frame++) {
        Mat data;
        video.read(data);

        auto [width, height] = get_terminal_size();
        height = height * 2 - 4;
        data = resize_mat(data, height, width);

        display_status_bar(curr_frame, total_frames, duration_seconds, fps, data.cols, data.rows);
#ifdef CURR_FRAME
        std::string to_display;
        to_display.reserve(width * height * 3);
        if (curr_frame == 1) {
            frame_drawer.init_currently_displayed(data);
            frame_drawer.display_entire_frame(to_display);
        } else {
            frame_drawer.process_new_frame(data, to_display);
        }
        fmt::detail::print(stdout, to_display);
#else
        draw_frame(data);
#endif
    }

    video.release();
}
