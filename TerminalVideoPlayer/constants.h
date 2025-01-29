#pragma once
#include <string>
#include <vector>
#include "Pixel.h"
#include <array>

const char esc = '\x1B';

constexpr const char *block = u8"\u2584"; // ? character
constexpr double default_optimization_threshold = 25.0;

constexpr std::string_view audio_file_name = "output_audio.wav";

constexpr int skip_seconds = 5;
constexpr long long nano_seconds_in_second = 1'000'000'000;

constexpr std::array<std::string_view, 8> block_chars {
    u8" ", u8"\u258F", u8"\u258E", u8"\u258D", u8"\u258C", u8"\u258B", u8"\u258A", u8"\u2589"
};

constexpr const char *full_block = u8"\u2588";

typedef std::vector<std::vector<TerminalPixel>> Frame;
