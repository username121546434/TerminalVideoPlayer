#include "Pixel.h"

Pixel::Pixel(cv::Vec3b pixel) : r {pixel[2]}, g {pixel[1]}, b {pixel[0]} {
}
