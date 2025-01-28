#pragma once
#include <string>
#include <vector>
#include "Pixel.h"
#include <memory>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class VideoDecoder {
public:
    VideoDecoder(const std::string &file_path);
    ~VideoDecoder();

    bool get_next_frame(std::unique_ptr<Pixel[]> &out_frame_data);
    void skip_to_timestamp(double timestamp_seconds);
    std::pair<int, int> resize_frame(const std::unique_ptr<Pixel[]> &input_frame_data, std::unique_ptr<Pixel[]> &output_frame_data, int max_width, int max_height);

    inline int get_width() const {
        return codec_context->width;
    }
    inline int get_height() const {
        return codec_context->height;
    }
    inline double get_fps() const {
        return fps;
    }
    inline long long get_total_frames() const {
        return total_frames;
    }
private:
    AVFormatContext *format_context = nullptr;
    AVCodecContext *codec_context = nullptr;
    AVCodecParameters *codec_parameters = nullptr;
    const AVCodec *codec = nullptr;
    AVFrame *frame = nullptr;
    AVFrame *frame_rgb = nullptr;
    SwsContext *sws_context = nullptr;
    int video_stream_index;
    std::vector<uint8_t> buffer;
    double fps;
    long long total_frames;
};

