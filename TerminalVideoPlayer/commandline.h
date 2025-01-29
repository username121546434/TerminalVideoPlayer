#pragma once
#include <string>
#include <tuple>

void print_help();
// return tuple of <redraw, optimization_threshold, video_file>
std::tuple<bool, double, std::string> parse_command_line(int argc, char *argv[]);
