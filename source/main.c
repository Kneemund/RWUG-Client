#include <whb/proc.h>
#include <whb/sdcard.h>
// #include <whb/log_console.h>
// #include <whb/log.h>
#include <coreinit/screen.h>
#include <coreinit/cache.h>
#include <vpad/input.h>

#include <arpa/inet.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <jansson.h>
#include <ini.h>

#include "udp_socket.h"
#include "dsu.h"

#define PORT 4242
#define DSU_PORT 26760

typedef struct {
    const char* ip_address;
} configuration;

static int configuration_handler(void* out, const char* section, const char* name, const char* value) {
    configuration* config = (configuration*) out;

    if (strcmp(section, "general") == 0) {
        if (strcmp(name, "ip_address") == 0) {
            config->ip_address = strdup(value);
        } else {
            return 0;
        }
    } else {
        return 0;
    }

    return 1;
}

void print_header(OSScreenID buffer) {
    OSScreenPutFontEx(buffer, 17, 1, "   _____      ___   _  ___ ");
    OSScreenPutFontEx(buffer, 17, 2, "  | _ \\ \\    / / | | |/ __|");
    OSScreenPutFontEx(buffer, 17, 3, "  |   /\\ \\/\\/ /| |_| | (_ |");
    OSScreenPutFontEx(buffer, 17, 4, "  |_|_\\ \\_/\\_/  \\___/ \\___|");
}

void reset_gyro_orientation() {
    VPADDirection identity_base = {
        { 1.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f }
    };

    VPADSetGyroAngle(VPAD_CHAN_0, 0.0f, 0.0f, 0.0f);
    VPADSetGyroDirection(VPAD_CHAN_0, &identity_base);
    VPADSetGyroDirReviseBase(VPAD_CHAN_0, &identity_base);
}

void pack_gamepad_data(VPADStatus* pad, char* buffer, uint32_t buffer_size) {
    json_t* data = json_object();

    VPADTouchData touchpad;
    VPADGetTPCalibratedPointEx(VPAD_CHAN_0, VPAD_TP_854X480, &touchpad, &pad->tpNormal);

    json_object_set_new_nocheck(data, "tp_touch", json_integer(touchpad.touched));
    json_object_set_new_nocheck(data, "tp_x", json_integer(touchpad.x));
    json_object_set_new_nocheck(data, "tp_y", json_integer(touchpad.y));

    json_object_set_new_nocheck(data, "hold", json_integer(pad->hold));
    json_object_set_new_nocheck(data, "release", json_integer(pad->release));
    json_object_set_new_nocheck(data, "trigger", json_integer(pad->trigger));
    json_object_set_new_nocheck(data, "battery", json_integer(pad->battery));
    json_object_set_new_nocheck(data, "l_stick_x", json_real(pad->leftStick.x));
    json_object_set_new_nocheck(data, "l_stick_y", json_real(pad->leftStick.y));
    json_object_set_new_nocheck(data, "r_stick_x", json_real(pad->rightStick.x));
    json_object_set_new_nocheck(data, "r_stick_y", json_real(pad->rightStick.y));
    json_object_set_new_nocheck(data, "gyro_x", json_real(pad->gyro.x));
    json_object_set_new_nocheck(data, "gyro_y", json_real(pad->gyro.y));
    json_object_set_new_nocheck(data, "gyro_z", json_real(pad->gyro.z));
    json_object_set_new_nocheck(data, "acc_x", json_real(pad->accelorometer.acc.x));
    json_object_set_new_nocheck(data, "acc_y", json_real(pad->accelorometer.acc.y));
    json_object_set_new_nocheck(data, "acc_z", json_real(pad->accelorometer.acc.z));

    char* str = json_dumps(data, JSON_REAL_PRECISION(12) | JSON_COMPACT);
    strncpy(buffer, str, buffer_size);

    free(str);
    json_decref(data);
}

int main(int argc, char **argv) {
    VPADStatus vpad_data;
    uint8_t raw_ip_address[4] = { 192, 168, 0, 1 };

    WHBProcInit();
    VPADInit();

    // VPADSetTVMenuInvalid(VPAD_CHAN_0, 1);

    WHBMountSdCard();
    char configuration_path[256];
    sprintf(configuration_path, "%s/wiiu/apps/RWUG/configuration.ini", WHBGetSdCardMountPath());

    OSScreenInit();

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


    // load & select IP address
    configuration config = { NULL };
    ini_parse(configuration_path, configuration_handler, &config);

    if (config.ip_address != NULL) {
        inet_pton(AF_INET, config.ip_address, &raw_ip_address);
        free((void*) config.ip_address);
    }

    char ip_print_buffer[16];
    uint8_t selection = 0;

    while (1) {
        VPADRead(VPAD_CHAN_0, &vpad_data, 1, NULL);
        if (vpad_data.trigger & (VPAD_BUTTON_LEFT  | VPAD_STICK_L_EMULATION_LEFT  | VPAD_STICK_R_EMULATION_LEFT ) && selection > 0) --selection;
        if (vpad_data.trigger & (VPAD_BUTTON_RIGHT | VPAD_STICK_L_EMULATION_RIGHT | VPAD_STICK_R_EMULATION_RIGHT) && selection < 3) ++selection;
        if (vpad_data.trigger & (VPAD_BUTTON_UP    | VPAD_STICK_L_EMULATION_UP    | VPAD_STICK_R_EMULATION_UP   )) raw_ip_address[selection] = (raw_ip_address[selection] < 255) ? (raw_ip_address[selection] + 1) : 0;
        if (vpad_data.trigger & (VPAD_BUTTON_DOWN  | VPAD_STICK_L_EMULATION_DOWN  | VPAD_STICK_R_EMULATION_DOWN )) raw_ip_address[selection] = (raw_ip_address[selection] >   0) ? (raw_ip_address[selection] - 1) : 255;

        OSScreenClearBufferEx(SCREEN_TV, 0x00000000);
        OSScreenClearBufferEx(SCREEN_DRC, 0x00000000);

        print_header(SCREEN_DRC);
        OSScreenPutFontEx(SCREEN_DRC, 0, 7, "Enter the IP address of the RWUG server below.");
        OSScreenPutFontEx(SCREEN_DRC, 0, 8, "Use the D-Pad or sticks to adjust the selection and its value.");

        sprintf(ip_print_buffer, "%3d.%3d.%3d.%3d", raw_ip_address[0], raw_ip_address[1], raw_ip_address[2], raw_ip_address[3]);
        OSScreenPutFontEx(SCREEN_DRC, 4 * selection, 10, "---");
        OSScreenPutFontEx(SCREEN_DRC, 0, 11, ip_print_buffer);
        OSScreenPutFontEx(SCREEN_DRC, 4 * selection, 12, "---");

        OSScreenPutFontEx(SCREEN_DRC, 0, 15, "A    - Confirm");
        OSScreenPutFontEx(SCREEN_DRC, 0, 16, "HOME - Exit");

        DCFlushRange(screenBufferTV, screenBufferSizeTV);
        DCFlushRange(screenBufferDRC, screenBufferSizeDRC);
        OSScreenFlipBuffersEx(SCREEN_TV);
        OSScreenFlipBuffersEx(SCREEN_DRC);

        if (vpad_data.trigger & VPAD_BUTTON_A) break;

        if (!WHBProcIsRunning()) {
            // cleanup
            OSScreenShutdown();
            free(screenBufferTV);
            free(screenBufferDRC);

            WHBUnmountSdCard();
            VPADShutdown();
            WHBProcShutdown();

            return 0;
        }
    }


    // setup
    reset_gyro_orientation();

    char ip_address[16];
    sprintf(ip_address, "%d.%d.%d.%d", raw_ip_address[0], raw_ip_address[1], raw_ip_address[2], raw_ip_address[3]);

    int udp_socket = init_udp_socket(DSU_PORT);

    char sending_string[64];
    sprintf(sending_string, "Sending UDP packets to %s:%d.", ip_address, PORT);

    OSScreenClearBufferEx(SCREEN_TV, 0x00000000);
    OSScreenClearBufferEx(SCREEN_DRC, 0x00000000);

    print_header(SCREEN_DRC);
    OSScreenPutFontEx(SCREEN_DRC, 0, 9, sending_string);
    OSScreenPutFontEx(SCREEN_DRC, 0, 10, "Make sure that the RWUG server is running on your computer.");
    OSScreenPutFontEx(SCREEN_DRC, 0, 16, "HOME - Exit");

    DCFlushRange(screenBufferTV, screenBufferSizeTV);
    DCFlushRange(screenBufferDRC, screenBufferSizeDRC);
    OSScreenFlipBuffersEx(SCREEN_TV);
    OSScreenFlipBuffersEx(SCREEN_DRC);


    // save configuration
    FILE* configuration_file = fopen(configuration_path, "w");
    if (configuration_file != NULL) {
        fprintf(configuration_file, "[general]\nip_address=%s\n\n", ip_address);
        fclose(configuration_file);
    }

    // WHBLogConsoleInit();


    // main loop
    char buffer[1024];
    struct timeval current_time;

    struct sockaddr_in connect_addr;
    memset(&connect_addr, 0, sizeof(connect_addr));

    connect_addr.sin_family = AF_INET;
    connect_addr.sin_port = htons(PORT);
    inet_aton(ip_address, &connect_addr.sin_addr);

    while (WHBProcIsRunning()) {
        VPADRead(VPAD_CHAN_0, &vpad_data, 1, NULL);

        pack_gamepad_data(&vpad_data, buffer, 1024);
        sendto(udp_socket, buffer, strlen(buffer), 0, (const struct sockaddr*) &connect_addr, sizeof(connect_addr));

        gettimeofday(&current_time, NULL);
        poll_dsu(&udp_socket, current_time.tv_sec * 1000000 + current_time.tv_usec, vpad_data.accelorometer.acc.x, vpad_data.accelorometer.acc.y, vpad_data.accelorometer.acc.z, vpad_data.gyro.x, vpad_data.gyro.y, vpad_data.gyro.z);
    }


    // cleanup
    destroy_udp_socket(&udp_socket);

    OSScreenShutdown();
    free(screenBufferTV);
    free(screenBufferDRC);

    WHBUnmountSdCard();
    VPADShutdown();
    // WHBLogConsoleFree();
    WHBProcShutdown();

    return 0;
}
