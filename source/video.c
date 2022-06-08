#include "video.h"

#include <whb/log.h>
#include <sys/time.h>

#include "common.h"

int video_open(struct VideoState* state, const char* path) {
    // av_log_set_level(AV_LOG_INFO);

    state->av_format_context = avformat_alloc_context();
    if (!state->av_format_context) {
        WHBLogPrint("Failed to open video: couldn't allocate format context.");
        return -1;
    }

    av_dict_set(&state->av_dictionary, "tcp_nodelay", "1", 0);

    int result = avformat_open_input(&state->av_format_context, path, NULL, &state->av_dictionary);
    if (result < 0) {
        WHBLogPrint("Failed to open video: couldn't open input.");
        WHBLogPrint(av_err2str(result));
        return -1;
    }

    WHBLogPrint("Opened avformat input.");

    state->av_format_context->max_analyze_duration = 50000;

    result = avformat_find_stream_info(state->av_format_context, NULL);
    if (result < 0) {
        WHBLogPrint("Failed to open video: couldn't find stream info.");
        WHBLogPrint(av_err2str(result));
        return -1;
    }

    state->video_stream_index = -1;
    AVCodecParameters* av_codec_parameters;
    const AVCodec* av_codec;

    for (int i = 0; i < state->av_format_context->nb_streams; ++i) {
        av_codec_parameters = state->av_format_context->streams[i]->codecpar;
        av_codec = avcodec_find_decoder(av_codec_parameters->codec_id);

        if (!av_codec) continue;

        if (av_codec_parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            state->video_stream_index = i;
            state->time_base = state->av_format_context->streams[i]->time_base;
            break;
        }
    }

    if (state->video_stream_index == -1) {
        WHBLogPrint("Failed to open video: couldn't find video stream.");
        return -1;
    }

    state->av_codec_context = avcodec_alloc_context3(av_codec);
    if (!state->av_codec_context) {
        WHBLogPrint("Failed to open video: couldn't allocate codec context.");
        return -1;
    }

    if (avcodec_parameters_to_context(state->av_codec_context, av_codec_parameters) < 0) {
        WHBLogPrint("Failed to open video: couldn't initialise codec context.");
        return -1;
    }

    if (avcodec_open2(state->av_codec_context, av_codec, NULL) < 0) {
        WHBLogPrint("Failed to open video: couldn't open codec.");
        return -1;
    }

    state->av_frame = av_frame_alloc();
    if (!state->av_frame) {
        WHBLogPrint("Failed to open video: couldn't allocate frame.");
        return -1;
    }

    state->av_packet = av_packet_alloc();
    if (!state->av_packet) {
        WHBLogPrint("Failed to open video: couldn't allocate packet.");
        return -1;
    }

    WHBLogPrintf("Height: %d, Width: %d", state->av_codec_context->height, state->av_codec_context->width);
    state->sws_context = sws_getContext(
        state->av_codec_context->width, state->av_codec_context->height, state->av_codec_context->pix_fmt,
        DRC_WIDTH, DRC_HEIGHT, AV_PIX_FMT_RGB0,
        SWS_FAST_BILINEAR,
        NULL, NULL, NULL
    );

    if (!state->sws_context) {
        WHBLogPrint("Failed to read frame: couldn't initialise swscaler.");
        return -1;
    }

    struct timeval start;
    gettimeofday(&start, NULL);
    state->start_microseconds = start.tv_sec * 1000000 + start.tv_usec;

    return 0;
}

int video_next_frame(struct VideoState* state, uint8_t* buffer, int64_t* presentationTS) {
    int result;

    while (av_read_frame(state->av_format_context, state->av_packet) >= 0) {
        if (state->av_packet->stream_index != state->video_stream_index) {
            av_packet_unref(state->av_packet);
            continue;
        }

        result = avcodec_send_packet(state->av_codec_context, state->av_packet);
        if (result < 0) {
            WHBLogPrintf("Failed to decode packet: %s", av_err2str(result));
            return -1;
        }

        result = avcodec_receive_frame(state->av_codec_context, state->av_frame);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
            av_packet_unref(state->av_packet);
            continue;
        } else if (result < 0) {
            WHBLogPrintf("Failed to receive frame: %s", av_err2str(result));
            return -1;
        }

        av_packet_unref(state->av_packet);
        break;
    }

    *presentationTS = state->av_frame->best_effort_timestamp;

    uint8_t* const destination_data[4] = { buffer, NULL, NULL, NULL };
    const int32_t destination_linesize[4] = { DRC_WIDTH * 4, 0, 0, 0 };
    sws_scale(state->sws_context, (const uint8_t**) state->av_frame->data, state->av_frame->linesize, 0, state->av_frame->height, destination_data, destination_linesize);

    return 0;
}

int video_close(struct VideoState* state) {
    sws_freeContext(state->sws_context);

    avformat_close_input(&state->av_format_context);
    avformat_free_context(state->av_format_context);

    av_frame_free(&state->av_frame);
    av_packet_free(&state->av_packet);
    avcodec_free_context(&state->av_codec_context);
    av_dict_free(&state->av_dictionary);

    return 0;
}