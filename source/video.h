#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>

struct VideoState {
    uint64_t start_microseconds;
    AVRational time_base;
    int video_stream_index;

    AVFormatContext* av_format_context;
    AVCodecContext* av_codec_context;
    AVDictionary* av_dictionary;
    AVFrame* av_frame;
    AVPacket* av_packet;
    struct SwsContext* sws_context;
};

int video_open(struct VideoState* state, const char* file);
int video_next_frame(struct VideoState* state, uint8_t* buffer, int64_t* pts);
int video_close(struct VideoState* state);