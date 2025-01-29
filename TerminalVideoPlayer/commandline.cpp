#include "commandline.h"
#include <iostream>
#include "constants.h"

void print_help() {
    std::cout << "Usage: TerminalVideoPlayer.exe [options] <video_file>" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help\t\t\tShow this help message and exit" << std::endl;
    std::cout << "  -o, --optimization-level\tOptimization level" << std::endl;
    std::cout << "                          \t(higher values lead to more artifacts but better performance" << std::endl;
    std::cout << "                          \tcan be a decimal value, default is " << default_optimization_threshold << std::endl;
    std::cout << "  -r, --redraw\t\t\tForce redraw of the entire frame instead of optimizing and only updating pixels that need updating" << std::endl;
    std::cout << "              \t\t\tDefault is false, should only be used if you hate artifacts, love slow powerpoint presentations," << std::endl;
    std::cout << "              \t\t\tor if your terminal font size is somewhat big" << std::endl;
}

std::tuple<bool, double, std::string> parse_command_line(int argc, char *argv[]) {
    if (argc < 2) {
        print_help();
        exit(1);
    }
    bool redraw = false;
    double optimization_threshold = default_optimization_threshold;
    std::string video_file;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_help();
            exit(0);
        } else if (arg == "-o" || arg == "--optimization-level") {
            if (i + 1 < argc) {
                optimization_threshold = std::stod(argv[i + 1]);
                if (optimization_threshold < 0) {
                    std::cerr << "Error: optimization threshold must be a positive number" << std::endl;
                    exit(1);
                }
                i++;
            } else {
                std::cerr << "Error: -o requires an argument" << std::endl;
                exit(1);
            }
        } else if (arg == "-r" || arg == "--redraw") {
            redraw = true;
        } else {
            video_file = arg;
        }
    }
    if (video_file.empty()) {
        std::cerr << "Error: no video file specified" << std::endl;
        exit(1);
    }
    return {redraw, optimization_threshold, video_file};
}
