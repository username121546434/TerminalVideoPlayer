#include "get_terminal_size.h"

// below is modified code from https://stackoverflow.com/a/62485211/19581763

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <Windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <sys/ioctl.h>
#endif // Windows/Linux

std::pair<int, int> get_terminal_size() {
#if defined(_WIN32)
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int width = (int)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
    int height = (int)(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
#elif defined(__linux__) || defined(__APPLE__)
    struct winsize w;
    ioctl(fileno(stdout), TIOCGWINSZ, &w);
    int width = (int)(w.ws_col);
    int height = (int)(w.ws_row);
#endif // Windows/Linux
    return std::make_pair(width, height);
}
