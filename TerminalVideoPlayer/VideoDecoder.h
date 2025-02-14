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

    // if heap_allocate is true, the frame data will be allocated on the heap
    // and the caller is reponsible to free it
    // otherwise just return a raw pointer to the internal ffmpeg data
    // in which case the caller should not free the memory
    const AVFrame *get_next_frame();
    long double skip_to_timestamp(double timestamp_seconds);
    std::pair<int, int> resize_frame(const AVFrame *input_frame, std::unique_ptr<const Pixel[]> &output_frame_data, int max_width, int max_height);

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
    AVPacket *packet = nullptr;
    AVFrame *resized_frame = nullptr;
    const AVCodec *codec = nullptr;
    AVFrame *frame = nullptr;
    AVFrame *frame_rgb = nullptr;
    SwsContext *sws_context = nullptr;
    int video_stream_index;
    std::vector<uint8_t> buffer;
    double fps;
    long long total_frames;
};

