#include "VideoDecoder.h"
#include <thread>
#include <iostream>
#include <stdexcept>

VideoDecoder::VideoDecoder(const std::string &file_path) {
    if (avformat_open_input(&format_context, file_path.c_str(), nullptr, nullptr) != 0) {
        throw std::runtime_error("Could not open video file.");
    }

    if (avformat_find_stream_info(format_context, nullptr) < 0) {
        throw std::runtime_error("Could not retrieve stream info.");
    }

    video_stream_index = -1;
    for (unsigned int i = 0; i < format_context->nb_streams; i++) {
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }

    if (video_stream_index == -1) {
        throw std::runtime_error("No video stream found.");
    }

    codec_parameters = format_context->streams[video_stream_index]->codecpar;
    codec = avcodec_find_decoder(codec_parameters->codec_id);
    if (!codec) {
        throw std::runtime_error("Unsupported codec.");
    }

    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
        throw std::runtime_error("Could not allocate codec context.");
    }

    if (avcodec_parameters_to_context(codec_context, codec_parameters) < 0) {
        throw std::runtime_error("Could not initialize codec context.");
    }

    if (avcodec_open2(codec_context, codec, nullptr) < 0) {
        throw std::runtime_error("Could not open codec.");
    }

    frame = av_frame_alloc();
    frame_rgb = av_frame_alloc();
    packet = av_packet_alloc();
    resized_frame = av_frame_alloc();
    if (!frame || !frame_rgb || !packet || !resized_frame) {
        throw std::runtime_error("Could not allocate frame or packet.");
    }

    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, codec_context->width, codec_context->height, 1);
    buffer.resize(num_bytes);

    av_image_fill_arrays(frame_rgb->data, frame_rgb->linesize, buffer.data(), AV_PIX_FMT_RGB24, codec_context->width, codec_context->height, 1);

    sws_context = sws_getContext(
        codec_context->width, codec_context->height, codec_context->pix_fmt,
        codec_context->width, codec_context->height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!sws_context) {
        throw std::runtime_error("Could not initialize SwsContext.");
    }

    fps = av_q2d(format_context->streams[video_stream_index]->r_frame_rate);
    total_frames = format_context->streams[video_stream_index]->nb_frames;

    codec_context->thread_count = std::thread::hardware_concurrency();
    codec_context->thread_type = FF_THREAD_FRAME;
}

VideoDecoder::~VideoDecoder() {
    av_frame_free(&frame);
    av_frame_free(&frame_rgb);
    av_frame_free(&resized_frame);
    av_packet_free(&packet);
    sws_freeContext(sws_context);
    avcodec_free_context(&codec_context);
    avformat_close_input(&format_context);
}

const AVFrame *VideoDecoder::get_next_frame() {
    while (av_read_frame(format_context, packet) >= 0) {
        if (packet->stream_index == video_stream_index) {
            if (avcodec_send_packet(codec_context, packet) == 0) {
                if (avcodec_receive_frame(codec_context, frame) == 0) {
                    sws_scale(sws_context, frame->data, frame->linesize, 0, codec_context->height, frame_rgb->data, frame_rgb->linesize);
                    av_packet_unref(packet);
                    return frame_rgb;
                }
            }
        }
        av_packet_unref(packet);
    }
    return nullptr;
}

long double VideoDecoder::skip_to_timestamp(double timestamp_seconds) {
    // std::cout << "Skipping to timestamp " << timestamp_seconds << " seconds." << std::endl;
    auto time = av_rescale(timestamp_seconds * 1000, format_context->streams[video_stream_index]->time_base.den, format_context->streams[video_stream_index]->time_base.num);
    time /= 1000;
    if (av_seek_frame(format_context, -1, timestamp_seconds * AV_TIME_BASE, AVSEEK_FLAG_BACKWARD) < 0) {
        throw std::runtime_error("Could not seek to the requested timestamp.");
    }
    avcodec_flush_buffers(codec_context);
    get_next_frame();
    auto timestamp_in_seconds_that_was_actually_seeked =
        (long double)format_context->streams[video_stream_index]->time_base.num * frame->best_effort_timestamp / format_context->streams[video_stream_index]->time_base.den;
    return timestamp_in_seconds_that_was_actually_seeked;
}

std::pair<int, int> VideoDecoder::resize_frame(const AVFrame *input_frame, std::unique_ptr<const Pixel[]> &output_frame_data, int max_width, int max_height) {
    double aspect_ratio = static_cast<double>(codec_context->width) / codec_context->height;
    int new_width = max_width;
    int new_height = max_height;

    if (max_width / aspect_ratio <= max_height) {
        new_height = static_cast<int>(max_width / aspect_ratio);
    } else {
        new_width = static_cast<int>(max_height * aspect_ratio);
    }

    SwsContext *resize_context = sws_getContext(
        codec_context->width, codec_context->height, AV_PIX_FMT_RGB24,
        new_width, new_height, AV_PIX_FMT_RGB24,
        SWS_BICUBIC, nullptr, nullptr, nullptr);

    if (!resize_context) {
        throw std::runtime_error("Could not initialize resize SwsContext.");
    }

    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, new_width, new_height, 1);
    auto data = std::unique_ptr<uint8_t[]>(new uint8_t[num_bytes]);
    av_image_fill_arrays(resized_frame->data, resized_frame->linesize, data.get(), AV_PIX_FMT_RGB24, new_width, new_height, 1);

    output_frame_data = std::unique_ptr<Pixel[]>(reinterpret_cast<Pixel *>(data.release()));

    sws_scale(
        resize_context,
        &input_frame->data[0], &input_frame->linesize[0], 0, codec_context->height,
        resized_frame->data, resized_frame->linesize);

    resized_frame->data[0] = nullptr; // to ensure that ffmpeg doesn't free the output data
    sws_freeContext(resize_context);

    return {new_width, new_height};
}
