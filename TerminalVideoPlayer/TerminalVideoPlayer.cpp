#include <iostream>
#include "AudioPlayer.h"
#include <conio.h>
#include <chrono>
#include <iomanip>
#include <string>
#include <locale>
#include <opencv2/videoio.hpp>
#include "get_terminal_size.h"
#include "utils.h"
#include <fmt/core.h>
#include <map>

#ifdef _WIN32
#include <windows.h>
// Function to enable virtual terminal processing
void EnableVirtualTerminalProcessing() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Invalid handle" << std::endl;
        return;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
        std::cerr << "Error: Unable to get console mode" << std::endl;
        return;
    }

    // Enable the virtual terminal processing mode
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) {
        std::cerr << "Error: Unable to set console mode" << std::endl;
    }
}
#endif

constexpr const char *block = u8"\u2584"; // ? character
constexpr double threshold = 25.0;

constexpr std::string_view audio_file_name = "output_audio.wav";

constexpr int skip_seconds = 5;

std::array<std::string_view, 8> block_chars {
    u8" ", u8"\u258F", u8"\u258E", u8"\u258D", u8"\u258C", u8"\u258B", u8"\u258A", u8"\u2589"
};

constexpr const char *full_block = u8"\u2588";

typedef std::vector<std::vector<TerminalPixel>> Frame;

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

void display_status_bar(int curr_frame, int total_frames, int duration_seconds, int fps, double curr_fps, int width, int height, double frames_to_drop) {
    int seconds_watched {curr_frame / fps};
    std::string result;
    set_cursor(0, 0, result);
    std::cout << result;
    std::cout << "\033[0mFrame " << curr_frame << "/" << total_frames << " " << width << "x" << height << " "
              << format_seconds(seconds_watched) << "/" << format_seconds(duration_seconds) << " "
              << std::setprecision(2) << std::fixed << curr_fps << "fps, frames to drop: " << frames_to_drop
              << '\n';
}

inline void print_pixel(TerminalPixel pixel, size_t x, size_t y, std::string &result, const Frame &currently_displayed) {
    set_color(pixel.top_pixel, true, result);
    set_color(pixel.bottom_pixel, false, result);

    result += block;

    if (x == currently_displayed[y].size() - 1) {
        result.push_back(esc);
        result += "[0m";
    }
}

void update_pixel(TerminalPixel new_pixel, bool move_cursor, size_t x, size_t y, std::string &result, Frame &currently_displayed, int padding_left) {
    currently_displayed[y][x] = new_pixel;
    if (move_cursor)
        set_cursor(x + padding_left, y + 2, result);
    print_pixel(new_pixel, x, y, result, currently_displayed);
}

void init_currently_displayed(const cv::Mat &start_frame, Frame &currently_displayed) {
    currently_displayed.reserve(start_frame.rows);
    for (int row = 0; row < start_frame.rows; row += 2) {
        std::vector<TerminalPixel> curr_row;
        curr_row.reserve(start_frame.cols);

        for (int col = 0; col < start_frame.cols; ++col) {
            Pixel top_pixel = start_frame.at<cv::Vec3b>(row, col);
            Pixel bottom_pixel = row + 1 < start_frame.rows ? start_frame.at<cv::Vec3b>(row + 1, col) : top_pixel;
            curr_row.emplace_back(top_pixel, bottom_pixel);
        }

        currently_displayed.push_back(curr_row);
    }
}

void process_new_frame(const cv::Mat &frame, std::string &result, Frame &currently_displayed, const std::string &left_padding) {
    int pixels {frame.rows * frame.cols};

    bool last_pixel_changed {false};

    for (size_t row = 0; row < currently_displayed.size(); row++) {
        const auto &curr_row {currently_displayed[row]};
        for (int col = 0; col < frame.cols; ++col) {
            TerminalPixel p {curr_row[col]};

            Pixel top_new_pixel {frame.at<cv::Vec3b>(row * 2, col)};
            Pixel bottom_new_pixel;
            if (row * 2 + 1 < frame.rows)
                bottom_new_pixel = frame.at<cv::Vec3b>(row * 2 + 1, col);
            else
                bottom_new_pixel = top_new_pixel;
            TerminalPixel new_p {top_new_pixel, bottom_new_pixel};

            if (distance(p.top_pixel, new_p.top_pixel) >= threshold ||
                distance(p.bottom_pixel, new_p.bottom_pixel) >= threshold
            ) {
                update_pixel(new_p, (!last_pixel_changed || (col == 0)), col, row, result, currently_displayed, left_padding.size());
                last_pixel_changed = true;
            } else
                last_pixel_changed = false;
        }
    }
}

void display_entire_frame(std::string &result, const Frame &currently_displayed, const std::string &left_padding) {
    size_t y = 0;
    for (const std::vector<TerminalPixel> &row : currently_displayed) {
        result += left_padding;
        size_t x = 0;
        for (TerminalPixel pixel : row) {
            print_pixel(pixel, x, y, result, currently_displayed);
            if (x == row.size() - 1) {
                result.push_back('\n');
            }
            x++;
        }
        y++;
    }
}

void draw_progressbar(int current_frame, int total_frames, int width) {
    double progress = (static_cast<double>(current_frame) / total_frames);
    int whole_width = std::floor(progress * width);
    double remainder_width = fmod(progress * width, 1.0);
    int part_width = std::floor(remainder_width * 8);

    const auto &partial_block_char = block_chars[part_width];

    std::string to_print {"\033[31m"};
    to_print.reserve(width * 4);
    for (int i = 0; i < width; ++i) {
        if (i < whole_width)
            to_print += full_block;
        else if (i == whole_width)
            to_print += partial_block_char;
        else
            to_print.push_back(' ');
    }
    fmt::print(to_print);
}

int main(int argc, char *argv[]) {

#ifdef _WIN32
    EnableVirtualTerminalProcessing();
#endif

    std::string file;
    std::setlocale(LC_ALL, "");
    
    std::cout << "\033[?1049h"; // save screen

    if (argc > 1)
        file = argv[1];
    else {
        std::cout << "Enter a video file: ";
        std::getline(std::cin, file);
    }

    // convert video to .wav file
    std::system(fmt::format("ffmpeg -i \"{}\" {}", file, audio_file_name).c_str());
    AudioPlayer audio_player {audio_file_name.data(), skip_seconds};

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
    double frames_to_drop {};
    int seek_frames = static_cast<int>(skip_seconds * fps); // Number of frames to seek for 5 seconds

    audio_player.get_sample_rate();

    double avg_fps {};

    Frame currently_displayed;

    int last_width {0};
    int last_height {0};

    bool should_redraw = false;

    std::string left_padding;

    audio_player.play();

    for (int curr_frame = 1; curr_frame < total_frames; ++curr_frame) {
        if (_kbhit()) {
            char key = _getch();
            if (key == ' ' || key == 'k') {
                // pause
                audio_player.pause();
                while (true) {
                    key = _getch();
                    if (key == ' ' || key == 'k')
                        break;
                    else if (key == 'q') {
                        curr_frame = total_frames;
                        break;
                    }
                }
                audio_player.play();
            } else if (key == 'l') {
                // seek forward
                curr_frame = std::min(curr_frame + seek_frames, static_cast<int>(total_frames) - 1);
                video.set(VideoCaptureProperties::CAP_PROP_POS_FRAMES, curr_frame);
                continue;
            } else if (key == 'j') {
                // seek backward
                curr_frame = std::max(curr_frame - seek_frames, 1);
                video.set(VideoCaptureProperties::CAP_PROP_POS_FRAMES, curr_frame);
                continue;
            } else if (key == 'q')
                break;
            else if (key == 'r')
                should_redraw = true;
        }
        auto startTime = std::chrono::high_resolution_clock::now();

        Mat data;
        video.read(data);

        auto [width, height] = get_terminal_size();
        height = height * 2 - 4;

        data = resize_mat(data, height, width);

        int padding_left = (width - data.cols) / 2;

        if (frames_to_drop > 1) {
            display_status_bar(curr_frame, total_frames, duration_seconds, fps, curr_fps, currently_displayed[0].size(), currently_displayed.size() * 2, frames_to_drop);
            frames_to_drop--;
            continue;
        }

        std::string to_display;
        to_display.reserve(width * height * 3);
        if (curr_frame == 1 || width != last_width || height != last_height || should_redraw) {
            left_padding.resize(padding_left, ' ');
            clear_screen();
            currently_displayed.clear();

            init_currently_displayed(data, currently_displayed);
            display_entire_frame(to_display, currently_displayed, left_padding);
            last_height = height;
            last_width = width;
            should_redraw = false;
        } else {
            process_new_frame(data, to_display, currently_displayed, left_padding);
        }
        const Frame &frame {currently_displayed};

        display_status_bar(curr_frame, total_frames, duration_seconds, fps, curr_fps, frame[0].size(), frame.size(), frames_to_drop);
        fmt::print(stdout, to_display);

        std::cout << "\033[0m\033[" << height - 1 << ";0H";
        draw_progressbar(curr_frame, total_frames, width);

        auto endTime = std::chrono::high_resolution_clock::now();
        auto elapsedTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        int sleepTimeMs = target_frame_time - static_cast<int>(elapsedTimeMs);
        if (sleepTimeMs > 0) {
            curr_fps = fps;
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMs));
        } else {
            curr_fps = 1.0 / (elapsedTimeMs / 1000.0);
            frames_to_drop += fps / curr_fps + 1;
        }

        if (curr_frame == 1)
            avg_fps = curr_fps;
        else
            avg_fps = (avg_fps * (curr_frame - 1) + curr_fps) / curr_frame;

        if (curr_frame % 5 == 0)
            audio_player.seek_to(curr_frame / fps);
    }

    video.release();
    fmt::print("\033[0m"); // resets terminal color so that the user can continue with the same window

#ifdef _WIN32
    std::system(fmt::format("del {}", audio_file_name).c_str());
#else
    std::system(fmt::format("rm {}", audio_file_name).c_str());
#endif

    std::cout << "\033[?1049l"; // restore screen

    std::cout << "Average FPS: " << avg_fps << std::endl;
}
