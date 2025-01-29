// disables min and max macros in windows.h
#define NOMINMAX
#include <iostream>
#include <thread>
#include "AudioPlayer.h"
#include <conio.h>
#include <chrono>
#include <iomanip>
#include <string>
#include <locale>
#include "VideoDecoder.h"
#include "get_terminal_size.h"
#include <array>
#include <sstream>
#include "utils.h"
#include <fmt/core.h>
#include <map>
#include <optional>

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
constexpr long long nano_seconds_in_second = 1'000'000'000;

std::array<std::string_view, 8> block_chars {
    u8" ", u8"\u258F", u8"\u258E", u8"\u258D", u8"\u258C", u8"\u258B", u8"\u258A", u8"\u2589"
};

constexpr const char *full_block = u8"\u2588";

typedef std::vector<std::vector<TerminalPixel>> Frame;

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

void display_status_bar(std::string &to_display, int curr_frame, int total_frames, int duration_seconds, int fps, double curr_fps, double avg_fps, int width, int height, double frames_to_drop) {
    int seconds_watched {curr_frame / fps};
    std::string status_bar;
    set_cursor(0, 0, status_bar);
    fmt::format_to(
        std::back_inserter(status_bar),
        "\033[0mFrame {}/{} {}x{} {}/{} {:.2f}fps, frames to drop: {:.2f} average fps: {:.2f}\n",
        curr_frame, total_frames, width, height,
        format_seconds(seconds_watched), format_seconds(duration_seconds),
        curr_fps, frames_to_drop, avg_fps
    );
    to_display = status_bar + to_display;
}

inline void print_pixel(TerminalPixel pixel, size_t x, size_t y, std::string &result, const Frame &currently_displayed, std::pair<bool, bool> change_bg_fg_color) {
    if (change_bg_fg_color.first)
        set_color(pixel.top_pixel, true, result);
    if (change_bg_fg_color.second)
        set_color(pixel.bottom_pixel, false, result);

    result += block;

    if (x == currently_displayed[y].size() - 1) {
        result.push_back(esc);
        result += "[0m";
    }
}

void update_pixel(TerminalPixel new_pixel, std::pair<bool, bool> change_bg_fg_color, bool move_cursor, size_t x, size_t y, std::string &result, Frame &currently_displayed, int padding_left) {
    currently_displayed[y][x] = new_pixel;
    if (move_cursor)
        set_cursor(x + padding_left, y + 2, result);
    print_pixel(new_pixel, x, y, result, currently_displayed, change_bg_fg_color);
}

void init_currently_displayed(const std::unique_ptr<Pixel[]> &start_frame, int rows, int cols, Frame &currently_displayed) {
    currently_displayed.reserve(rows);
    for (int row = 0; row < rows; row += 2) {
        std::vector<TerminalPixel> curr_row;
        curr_row.reserve(cols);

        for (int col = 0; col < cols; ++col) {
            Pixel top_pixel = start_frame[row * cols + col];
            Pixel bottom_pixel = row + 1 < rows ? start_frame[(row + 1) * cols + col] : top_pixel;
            curr_row.emplace_back(top_pixel, bottom_pixel);
        }

        currently_displayed.push_back(curr_row);
    }
}

void process_new_frame(const std::unique_ptr<Pixel[]> &frame, size_t rows, int cols, std::string &result, Frame &currently_displayed, const std::string &left_padding) {
    bool last_pixel_changed {false};
    std::optional<TerminalPixel> last_p;
    for (size_t row = 0; row < currently_displayed.size(); row++) {
        const auto &curr_row {currently_displayed[row]};
        for (int col = 0; col < cols; ++col) {
            TerminalPixel p {curr_row[col]};

            Pixel top_new_pixel {frame[(row * 2) * cols + col]};
            Pixel bottom_new_pixel;
            if (row * 2 + 1 < rows)
                bottom_new_pixel = frame[(row * 2 + 1) * cols + col];
            else
                bottom_new_pixel = top_new_pixel;
            TerminalPixel new_p {top_new_pixel, bottom_new_pixel};

            if (distance(p.top_pixel, new_p.top_pixel) >= threshold ||
                distance(p.bottom_pixel, new_p.bottom_pixel) >= threshold
            ) {
                bool should_move_cursor = (!last_pixel_changed || (col == 0));
                std::pair<bool, bool> change_bg_fg_color {false, false};
                if (last_p.has_value())
                    change_bg_fg_color = {last_p->top_pixel != new_p.top_pixel, last_p->bottom_pixel != new_p.bottom_pixel};
                else {
                    change_bg_fg_color = {true, true};
                    last_p = new_p;
                }
                if (should_move_cursor) {
                    change_bg_fg_color.first = true;
                    change_bg_fg_color.second = true;
                }
                update_pixel(
                    new_p,
                    change_bg_fg_color,
                    should_move_cursor,
                    col,
                    row,
                    result,
                    currently_displayed,
                    left_padding.size()
                );
                last_pixel_changed = true;
                if (last_p.has_value()) {
                    if (change_bg_fg_color.first)
                        last_p->top_pixel = new_p.top_pixel;
                    if (change_bg_fg_color.second)
                        last_p->bottom_pixel = new_p.bottom_pixel;
                }
            } else {
                last_pixel_changed = false;
            }
        }
    }
}

void display_entire_frame(std::string &result, const Frame &currently_displayed, const std::string &left_padding) {
    size_t y = 0;
    for (const std::vector<TerminalPixel> &row : currently_displayed) {
        result += left_padding;
        size_t x = 0;
        TerminalPixel last_pixel;
        for (TerminalPixel pixel : row) {
            std::pair<bool, bool> change_bg_fg_color {false, false};
            if (pixel.top_pixel != last_pixel.top_pixel)
                change_bg_fg_color.first = true;
            if (pixel.bottom_pixel != last_pixel.bottom_pixel)
                change_bg_fg_color.second = true;

            print_pixel(pixel, x, y, result, currently_displayed, change_bg_fg_color);
            if (x == row.size() - 1) {
                result.push_back('\n');
            }
            last_pixel = pixel;
            x++;
        }
        y++;
    }
}

void draw_progressbar(int current_frame, int total_frames, int width, std::string &to_display) {
    double progress = (static_cast<double>(current_frame) / total_frames);
    int whole_width = std::floor(progress * width);
    double remainder_width = fmod(progress * width, 1.0);
    int part_width = std::floor(remainder_width * 8);

    const auto &partial_block_char = block_chars[part_width];

    to_display += "\033[31m";
    to_display.reserve(width * 4);
    for (int i = 0; i < width; ++i) {
        if (i < whole_width)
            to_display += full_block;
        else if (i == whole_width)
            to_display += partial_block_char;
        else
            to_display.push_back(' ');
    }
}

int main(int argc, char *argv[]) {

#ifdef _WIN32
    EnableVirtualTerminalProcessing();
#endif

    std::string file;
    std::setlocale(LC_ALL, "");

    std::cout << "\033[?1049h"; // save current terminal content to restore later

    const auto temp_directory {create_temp_directory()};

    if (argc > 1)
        file = argv[1];
    else {
        std::cout << "Enter a video file: ";
        std::getline(std::cin, file);
    }

    // convert video to .wav file so miniaudio can play it
    const auto actual_audio_file = (temp_directory / audio_file_name);
    std::system(fmt::format("ffmpeg -i \"{}\" \"{}\"", file, actual_audio_file.string()).c_str());

    AudioPlayer audio_player {actual_audio_file.string().c_str(), skip_seconds};

    VideoDecoder video {file};

    long long total_frames {video.get_total_frames()};
    double fps {video.get_fps()};
    double duration_seconds {total_frames / fps};
    double curr_fps {};
    const std::chrono::nanoseconds target_frame_time {static_cast<long long>((1.0 / fps) * nano_seconds_in_second)};
    auto next_target_frame_time = target_frame_time;
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
    std::string to_display;

    std::chrono::nanoseconds last_elapsed_time;

    audio_player.play();

    for (long long curr_frame = 1; curr_frame < total_frames; ++curr_frame) {
        auto startTime = std::chrono::steady_clock::now();
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
                curr_frame = std::min(curr_frame + seek_frames, total_frames - 1);
                curr_frame = video.skip_to_timestamp(curr_frame / fps) * fps;
                continue;
            } else if (key == 'j') {
                // seek backward
                curr_frame = std::max(curr_frame - seek_frames, 1ll);
                curr_frame = video.skip_to_timestamp(curr_frame / fps) * fps;
                continue;
            } else if (key == 'q')
                break;
            else if (key == 'r')
                should_redraw = true;
            startTime = std::chrono::steady_clock::now();
        }

        const Pixel *data = video.get_next_frame(false);

        if (!data) {
            std::cerr << "Failed to get next frame" << std::endl;
            break;
        }

        if (frames_to_drop > 1) {
            display_status_bar(to_display, curr_frame, total_frames, duration_seconds, fps, curr_fps, avg_fps, currently_displayed[0].size(), currently_displayed.size() * 2, frames_to_drop);
            fmt::print(stdout, to_display);
            to_display.clear();
            frames_to_drop--;

            continue;
        }

        auto [width, height] = get_terminal_size();
        height = height * 2 - 4;

        std::unique_ptr<Pixel[]> new_data = nullptr;
        // the const cast should be safe since the original ffmpeg data
        // is not defined as const
        auto [actual_width, actual_height] = video.resize_frame(const_cast<Pixel*>(data), new_data, width, height);

        int padding_left = (width - actual_width) / 2;

        if (curr_frame == 1 || width != last_width || height != last_height || should_redraw) {
            to_display.reserve(width * height * 3);
            left_padding.resize(padding_left, ' ');
            clear_screen();
            currently_displayed.clear();

            init_currently_displayed(new_data, actual_height, actual_width, currently_displayed);
            display_entire_frame(to_display, currently_displayed, left_padding);
            last_height = height;
            last_width = width;
            should_redraw = false;
        } else {
            process_new_frame(new_data, actual_height, actual_width, to_display, currently_displayed, left_padding);
        }

        display_status_bar(to_display, curr_frame, total_frames, duration_seconds, fps, curr_fps, avg_fps, currently_displayed[0].size(), currently_displayed.size() * 2, frames_to_drop);

        fmt::format_to(std::back_inserter(to_display), "\033[0m\033[{};0H", height - 1);
        draw_progressbar(curr_frame, total_frames, width, to_display);

        fmt::print(to_display);
        to_display.clear();

        if (curr_frame == 1)
            avg_fps = curr_fps;
        else
            avg_fps = (avg_fps * (curr_frame - 1) + curr_fps) / curr_frame;

        if (curr_frame % 5 == 0)
            audio_player.seek_to(curr_frame / fps);

        auto endTime = std::chrono::steady_clock::now();
        auto elapsed_time_ns = (endTime - startTime);
        last_elapsed_time = elapsed_time_ns;
        auto sleep_time = next_target_frame_time - elapsed_time_ns;
        if (sleep_time.count() > 0) {
            curr_fps = fps;
            auto start = std::chrono::steady_clock::now();
            std::this_thread::sleep_for(sleep_time);
            auto actual_sleep_time = std::chrono::steady_clock::now() - start;
            next_target_frame_time = target_frame_time - (actual_sleep_time - sleep_time);
        } else {
            curr_fps = 1.0 / ((double)elapsed_time_ns.count() / nano_seconds_in_second);
            if (fps > curr_fps)
                frames_to_drop += fps / curr_fps;
            if (actual_width * actual_height > 400'000)
                frames_to_drop++;
            next_target_frame_time = target_frame_time;
        }
    }

    fmt::print("\033[0m"); // resets terminal color so that the user can continue with the same window

    std::filesystem::remove_all(temp_directory);

    std::cout << "\033[?1049l"; // restore whatever was on the terminal screen before

    std::cout << "Average FPS: " << avg_fps << std::endl;
}
