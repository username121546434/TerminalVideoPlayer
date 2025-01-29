#pragma once
#include "Pixel.h"
#include <filesystem>
#include <random>
#include <string>
#include "constants.h"

double distance(Pixel p1, Pixel p2);

std::filesystem::path create_temp_directory(unsigned long long max_tries = 100);

void set_cursor(size_t x, size_t y, std::string &result);
void set_color(Pixel p, bool bg, std::string &result);
void clear_screen();
