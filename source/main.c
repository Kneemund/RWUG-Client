#include <whb/proc.h>
#include <whb/sdcard.h>
#include <coreinit/screen.h>
#include <coreinit/cache.h>
#include <coreinit/thread.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#include "configuration.h"
#include "udp_socket.h"
#include "dsu.h"
#include "rwug.h"

#define DSU_PORT 26760
#define RWUG_PORT 4242

#define DATA_UPDATE_RATE 10000

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
    WHBProcInit();
    WHBMountSdCard();
    VPADInit();
    OSScreenInit();

    // VPADSetTVMenuInvalid(VPAD_CHAN_0, 1);

    const uint32_t screenBufferSizeTV = OSScreenGetBufferSizeEx(SCREEN_TV);
    const uint32_t screenBufferSizeDRC = OSScreenGetBufferSizeEx(SCREEN_DRC);
    void* screenBufferTV = memalign(0x100, screenBufferSizeTV);
    void* screenBufferDRC = memalign(0x100, screenBufferSizeDRC);

    OSScreenSetBufferEx(SCREEN_TV, screenBufferTV);
    OSScreenSetBufferEx(SCREEN_DRC, screenBufferDRC);
    OSScreenEnableEx(SCREEN_TV, 1);
    OSScreenEnableEx(SCREEN_DRC, 1);
    OSScreenClearBufferEx(SCREEN_TV, 0x00000000);
    OSScreenClearBufferEx(SCREEN_DRC, 0x00000000);



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

        DCFlushRange(screenBufferTV, screenBufferSizeTV);
        DCFlushRange(screenBufferDRC, screenBufferSizeDRC);
        OSScreenFlipBuffersEx(SCREEN_TV);
        OSScreenFlipBuffersEx(SCREEN_DRC);

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

    const bool enable_rwug = mode == 0 || mode == 2;
    const bool enable_dsu  = mode == 0 || mode == 1;

    int udp_socket = init_udp_socket(DSU_PORT);

    OSScreenClearBufferEx(SCREEN_TV, 0x00000000);
    OSScreenClearBufferEx(SCREEN_DRC, 0x00000000);

    print_header(SCREEN_DRC);

    uint8_t line = 9;
    char sending_string[64];
    if (enable_rwug) {
        sprintf(sending_string, "Sending data to RWUG server at %s:%d.", ip_address, RWUG_PORT);
        OSScreenPutFontEx(SCREEN_DRC, 0, line++, sending_string);
    }
    if (enable_dsu) {
        sprintf(sending_string, "Listening to DSU requests on %d.", DSU_PORT);
        OSScreenPutFontEx(SCREEN_DRC, 0, line++, sending_string);
    }

    OSScreenPutFontEx(SCREEN_DRC, 0, 16, "HOME - Exit");

    DCFlushRange(screenBufferTV, screenBufferSizeTV);
    DCFlushRange(screenBufferDRC, screenBufferSizeDRC);
    OSScreenFlipBuffersEx(SCREEN_TV);
    OSScreenFlipBuffersEx(SCREEN_DRC);

    save_configuration(configuration_path, ip_address, mode);

    struct sockaddr_in rwug_server_address;
    socklen_t rwug_server_address_size = sizeof(rwug_server_address);
    memset(&rwug_server_address, 0, rwug_server_address_size);

    rwug_server_address.sin_family = AF_INET;
    rwug_server_address.sin_port = htons(RWUG_PORT);
    inet_pton(AF_INET, ip_address, &rwug_server_address.sin_addr);



    struct timeval current_time;

    while (WHBProcIsRunning()) {
        VPADStatus pad_data;
        VPADRead(VPAD_CHAN_0, &pad_data, 1, NULL);

        VPADTouchData touchpad_data;
        VPADGetTPCalibratedPointEx(VPAD_CHAN_0, VPAD_TP_854X480, &touchpad_data, &pad_data.tpNormal);

        gettimeofday(&current_time, NULL);
        uint64_t microseconds = current_time.tv_sec * 1000000 + current_time.tv_usec;

        if (enable_rwug) update_rwug(&udp_socket, &pad_data, &touchpad_data, &microseconds, (const struct sockaddr*) &rwug_server_address, rwug_server_address_size);
        if (enable_dsu) update_dsu(&udp_socket, &microseconds, &pad_data, &touchpad_data);

        OSSleepTicks(OSMicrosecondsToTicks(DATA_UPDATE_RATE));
    }



    destroy_udp_socket(&udp_socket);

    OSScreenShutdown();
    free(screenBufferTV);
    free(screenBufferDRC);

    VPADShutdown();
    WHBUnmountSdCard();
    WHBProcShutdown();

    return 0;
}
