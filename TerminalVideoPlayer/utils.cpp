#include "utils.h"
#include <math.h>
#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <fmt/core.h>
#include <algorithm>

double distance(Pixel p1, Pixel p2) {
    return std::sqrt(std::pow(p1.r - p2.r, 2) + std::pow(p1.g - p2.g, 2) + std::pow(p1.b - p2.b, 2));
}

// taken from https://stackoverflow.com/a/58454949/19581763
std::filesystem::path create_temp_directory(unsigned long long max_tries) {
    auto tmp_dir = std::filesystem::temp_directory_path();
    unsigned long long i = 0;
    std::random_device dev;
    std::mt19937 prng(dev());
    std::uniform_int_distribution<uint64_t> rand(0);
    std::filesystem::path path;
    while (true) {
        std::stringstream ss;
        ss << std::hex << rand(prng);
        path = tmp_dir / ss.str();
        // true if the directory was created.
        if (std::filesystem::create_directory(path)) {
            break;
        }
        if (i == max_tries) {
            throw std::runtime_error("could not find non-existing directory");
        }
        i++;
    }
    return path;
}

void set_cursor(size_t x, size_t y, std::string &result) {
    fmt::format_to(std::back_inserter(result), "{}[{};{}H", esc, y, x + 1);
}

void set_color(Pixel p, bool bg, std::string &result) {
    fmt::format_to(std::back_inserter(result), "{}[{};2;{};{};{}m", esc, bg ? 48 : 38, p.r, p.g, p.b);
}

void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}
