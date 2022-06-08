#include <whb/proc.h>
#include <whb/sdcard.h>
#include <whb/log_udp.h>
#include <whb/log.h>
#include <vpad/input.h>
#include <coreinit/screen.h>
#include <coreinit/cache.h>
#include <malloc.h>
#include <sys/time.h>
#include <sys/iosupport.h>

#include "configuration.h"
#include "udp_socket.h"
#include "video.h"
#include "thread.h"
#include "input_server.h"
#include "common.h"

// SOUND: https://github.com/xhp-creations/WUP-Installer-GX2/blob/5105b58fa9ecb09add37d78eb1f1dd3976d21532/src/sounds/Voice.h
// https://github.com/Zarklord/Saving-U/blob/fb2ce5fbeef222afed02926fe89668a3fa93b597/src/sounds/SoundHandler.cpp
// https://github.com/BTTRG/AmePushesP/blob/fd07c48fa8c02fd6114d733ce281150c5fb4c927/src/Backends/Audio/SoftwareMixer/WiiU.cpp
// https://github.com/devkitPro/wut-packages/blob/dd4e4d7cb1064caef074bba768e21b70b4590d0b/SDL2/SDL2-2.0.9.patch#L642

static ssize_t wiiu_log_write(struct _reent* r, void* fd, const char* ptr, size_t len) {
    (void)r; (void)fd;
    WHBLogPrintf("%*.*s", len, len, ptr);
    return len;
}
static devoptab_t dotab_stdout = {
    .name = "udp_out",
    .write_r = &wiiu_log_write,
};

struct ScreenBuffer {
    OSScreenID screen;
    int num;
    unsigned int *image;
    size_t size;
    unsigned int width;
    unsigned int height;
};

static struct ScreenBuffer BUFFERS[2][2] = {
    {
        { .screen = SCREEN_TV, .num = 0, .width = 1280, .height = 720 },
        { .screen = SCREEN_TV, .num = 1, .width = 1280, .height = 720 },
    },
    {
        { .screen = SCREEN_DRC, .num = 0, .width = DRC_WIDTH, .height = DRC_HEIGHT },
        { .screen = SCREEN_DRC, .num = 1, .width = DRC_WIDTH, .height = DRC_HEIGHT },
    },
};
static int currentBuffer;

void ScreenFlip() {
    struct ScreenBuffer bufferTV = BUFFERS[SCREEN_TV][currentBuffer];
    DCFlushRange(bufferTV.image, bufferTV.size);
    OSScreenFlipBuffersEx(SCREEN_TV);

    struct ScreenBuffer bufferDRC = BUFFERS[SCREEN_DRC][currentBuffer];
    DCFlushRange(bufferDRC.image, bufferDRC.size);
    OSScreenFlipBuffersEx(SCREEN_DRC);

    currentBuffer = !currentBuffer;
}

void print_header(const OSScreenID buffer) {
    OSScreenPutFontEx(buffer, 19, 1, " _____      ___   _  ___ ");
    OSScreenPutFontEx(buffer, 19, 2, "| _ \\ \\    / / | | |/ __|");
    OSScreenPutFontEx(buffer, 19, 3, "|   /\\ \\/\\/ /| |_| | (_ |");
    OSScreenPutFontEx(buffer, 19, 4, "|_|_\\ \\_/\\_/  \\___/ \\___|");
}

void reset_gyro_orientation() {
    VPADDirection identity_base = {
        { 1.0, 0.0, 0.0 },
        { 0.0, 1.0, 0.0 },
        { 0.0, 0.0, 1.0 }
    };

    VPADSetGyroAngle(VPAD_CHAN_0, 0.0, 0.0, 0.0);
    VPADSetGyroDirection(VPAD_CHAN_0, &identity_base);
    VPADSetGyroDirReviseBase(VPAD_CHAN_0, &identity_base);
}

int main() {
    WHBLogUdpInit();
    devoptab_list[STD_OUT] = &dotab_stdout;
    devoptab_list[STD_ERR] = &dotab_stdout;

    WHBProcInit();
    WHBMountSdCard();
    VPADInit();
    OSScreenInit();

    WHBLogPrint("Initialised function pointers.");

    // VPADSetTVMenuInvalid(VPAD_CHAN_0, 1);

    const uint32_t screenBufferSizeTV = OSScreenGetBufferSizeEx(SCREEN_TV);
    const uint32_t screenBufferSizeDRC = OSScreenGetBufferSizeEx(SCREEN_DRC);
    void* screenBufferTV = memalign(0x100, screenBufferSizeTV);
    void* screenBufferDRC = memalign(0x100, screenBufferSizeDRC);

    BUFFERS[SCREEN_TV][0].image = screenBufferTV;
    BUFFERS[SCREEN_TV][1].image = screenBufferTV + (screenBufferSizeTV / 2);
    BUFFERS[SCREEN_TV][0].size = (screenBufferSizeTV / 2);
    BUFFERS[SCREEN_TV][1].size = (screenBufferSizeTV / 2);

    BUFFERS[SCREEN_DRC][0].image = screenBufferDRC;
    BUFFERS[SCREEN_DRC][1].image = screenBufferDRC + (screenBufferSizeDRC / 2);
    BUFFERS[SCREEN_DRC][0].size = (screenBufferSizeDRC / 2);
    BUFFERS[SCREEN_DRC][1].size = (screenBufferSizeDRC / 2);

    currentBuffer = 1;

    OSScreenSetBufferEx(SCREEN_TV, screenBufferTV);
    OSScreenSetBufferEx(SCREEN_DRC, screenBufferDRC);
    OSScreenEnableEx(SCREEN_TV, 1);
    OSScreenEnableEx(SCREEN_DRC, 1);



    char configuration_path[128];
    get_configuration_path(configuration_path);
    configuration config = load_configuration(configuration_path);

    uint8_t raw_ip_address[4];
    inet_pton(AF_INET, config.ip_address, &raw_ip_address);

    uint8_t mode = config.mode;
    const char* mode_to_string[] = { "DSU & Virtual Controller", "DSU", "Virtual Controller" };



    char print_buffer[64];
    uint8_t selection = 0;

    while (1) {
        VPADStatus pad_data;
        VPADRead(VPAD_CHAN_0, &pad_data, 1, NULL);

        if (pad_data.trigger & (VPAD_BUTTON_LEFT  | VPAD_STICK_L_EMULATION_LEFT  | VPAD_STICK_R_EMULATION_LEFT ) && selection > 0) --selection;
        if (pad_data.trigger & (VPAD_BUTTON_RIGHT | VPAD_STICK_L_EMULATION_RIGHT | VPAD_STICK_R_EMULATION_RIGHT) && selection < 4) ++selection;

        if (selection < 4) {
            if (pad_data.trigger & (VPAD_BUTTON_UP   | VPAD_STICK_L_EMULATION_UP   | VPAD_STICK_R_EMULATION_UP  )) raw_ip_address[selection] = (raw_ip_address[selection] < 255) ? (raw_ip_address[selection] + 1) : 0;
            if (pad_data.trigger & (VPAD_BUTTON_DOWN | VPAD_STICK_L_EMULATION_DOWN | VPAD_STICK_R_EMULATION_DOWN)) raw_ip_address[selection] = (raw_ip_address[selection] >   0) ? (raw_ip_address[selection] - 1) : 255;
        } else {
            if (pad_data.trigger & (VPAD_BUTTON_UP   | VPAD_STICK_L_EMULATION_UP   | VPAD_STICK_R_EMULATION_UP  )) mode = (mode < 2) ? (mode + 1) : 0;
            if (pad_data.trigger & (VPAD_BUTTON_DOWN | VPAD_STICK_L_EMULATION_DOWN | VPAD_STICK_R_EMULATION_DOWN)) mode = (mode > 0) ? (mode - 1) : 2;
        }

        OSScreenClearBufferEx(SCREEN_TV, 0x00000000);
        OSScreenClearBufferEx(SCREEN_DRC, 0x00000000);

        print_header(SCREEN_DRC);
        OSScreenPutFontEx(SCREEN_DRC, 0, 7, "Use the D-Pad or sticks to adjust the selection and its value.");

        sprintf(print_buffer, "RWUG IP   %3d.%3d.%3d.%3d", raw_ip_address[0], raw_ip_address[1], raw_ip_address[2], raw_ip_address[3]);
        OSScreenPutFontEx(SCREEN_DRC, 0, 10, print_buffer);

        sprintf(print_buffer, "Mode      %s", mode_to_string[mode]);
        OSScreenPutFontEx(SCREEN_DRC, 0, 12, print_buffer);

        if (selection < 4) OSScreenPutFontEx(SCREEN_DRC, 10 + 4 * selection, 11, "---");
        else OSScreenPutFontEx(SCREEN_DRC, 10, 13, "------------------------");

        OSScreenPutFontEx(SCREEN_DRC, 0, 15, "A    - Confirm");
        OSScreenPutFontEx(SCREEN_DRC, 0, 16, "HOME - Exit");

        ScreenFlip();

        if (pad_data.trigger & VPAD_BUTTON_A) break;

        if (!WHBProcIsRunning()) {
            OSScreenShutdown();
            free(screenBufferTV);
            free(screenBufferDRC);

            VPADShutdown();
            WHBUnmountSdCard();
            WHBProcShutdown();

            return 0;
        }
    }



    reset_gyro_orientation();

    char ip_address[16];
    sprintf(ip_address, "%d.%d.%d.%d", raw_ip_address[0], raw_ip_address[1], raw_ip_address[2], raw_ip_address[3]);



    struct InputServerArguments input_server_arguments = {
        .enable_rwug = mode == 0 || mode == 2,
        .enable_dsu = mode == 0 || mode == 1,
        .socket = init_udp_socket(DSU_PORT)
    };

    if (input_server_arguments.socket < 0) {
        WHBLogPrint("Creating UDP socket failed.");

        OSScreenShutdown();
        free(screenBufferTV);
        free(screenBufferDRC);

        VPADShutdown();
        WHBUnmountSdCard();
        WHBProcShutdown();
        WHBLogUdpDeinit();

        return -1;
    }

    OSScreenClearBufferEx(SCREEN_TV, 0x00000000);
    OSScreenClearBufferEx(SCREEN_DRC, 0x00000000);



    save_configuration(configuration_path, ip_address, mode);

    memset(&input_server_arguments.address, 0, sizeof(struct sockaddr_in));
    input_server_arguments.address.sin_family = AF_INET;
    input_server_arguments.address.sin_port = htons(RWUG_PORT);
    inet_pton(AF_INET, ip_address, &input_server_arguments.address.sin_addr);


    OSThread* input_server_thread;
    if (create_thread(&input_server_thread, &input_server, (void*) &input_server_arguments) != 0) {
        WHBLogPrint("Failed to create input server thread.");

        destroy_udp_socket(&input_server_arguments.socket);

        OSScreenShutdown();
        free(screenBufferTV);
        free(screenBufferDRC);

        VPADShutdown();
        WHBUnmountSdCard();
        WHBProcShutdown();
        WHBLogUdpDeinit();

        return -1;
    }



    // e.g. tcp://192.168.0.11:6100
    char video_path[32];
    sprintf(video_path, "tcp://%s:6100", ip_address);

    WHBLogPrint("Opening video...");

    struct VideoState video_state = {};
    if (video_open(&video_state, video_path) < 0) {
        WHBLogPrint("Opening video failed.");

        close_input_server(input_server_thread);
        video_close(&video_state);
        destroy_udp_socket(&input_server_arguments.socket);

        OSScreenShutdown();
        free(screenBufferTV);
        free(screenBufferDRC);

        VPADShutdown();
        WHBUnmountSdCard();
        WHBProcShutdown();
        WHBLogUdpDeinit();

        return -1;
    }

    WHBLogPrint("Opened video.");



    struct timeval current_time;
    while (WHBProcIsRunning()) {
        gettimeofday(&current_time, NULL);
        uint64_t microseconds = current_time.tv_sec * 1000000 + current_time.tv_usec;

        struct ScreenBuffer workBuffer = BUFFERS[SCREEN_DRC][currentBuffer];

        int64_t presentationTS;
        if (video_next_frame(&video_state, (uint8_t*) workBuffer.image, &presentationTS) < 0) {
            WHBLogPrint("Failed to get frame.");

            close_input_server(input_server_thread);
            video_close(&video_state);
            destroy_udp_socket(&input_server_arguments.socket);

            OSScreenShutdown();
            free(screenBufferTV);
            free(screenBufferDRC);

            VPADShutdown();
            WHBUnmountSdCard();
            WHBProcShutdown();
            WHBLogUdpDeinit();

            return -1;
        }

        double presentation_seconds = presentationTS * (double) video_state.time_base.num / (double) video_state.time_base.den;
        double sync_sleep_microseconds = 1000000.0 * presentation_seconds - ((double) (microseconds - video_state.start_microseconds));
        if (sync_sleep_microseconds > 0) OSSleepTicks(OSMicrosecondsToTicks(sync_sleep_microseconds));

        ScreenFlip();
    }

    close_input_server(input_server_thread);
    video_close(&video_state);
    destroy_udp_socket(&input_server_arguments.socket);

    OSScreenShutdown();
    free(screenBufferTV);
    free(screenBufferDRC);

    VPADShutdown();
    WHBUnmountSdCard();
    WHBProcShutdown();
    WHBLogUdpDeinit();

    return 0;
}
