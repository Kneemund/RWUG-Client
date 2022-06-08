#include "input_server.h"

#include <sys/time.h>
#include <whb/log.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>

#include "dsu.h"
#include "rwug.h"
#include "common.h"

static bool run_input_server = true;

void* input_server(void* args) {
    struct InputServerArguments* server_arguments = args;

    WHBLogPrint("Started input server thread.");

    struct timeval current_time;
    while (run_input_server) {
        VPADStatus pad_data;
        VPADRead(VPAD_CHAN_0, &pad_data, 1, NULL);

        VPADTouchData touchpad_data;
        VPADGetTPCalibratedPointEx(VPAD_CHAN_0, VPAD_TP_854X480, &touchpad_data, &pad_data.tpNormal);

        gettimeofday(&current_time, NULL);
        uint64_t microseconds = current_time.tv_sec * 1000000 + current_time.tv_usec;

        if (server_arguments->enable_rwug) update_rwug(&server_arguments->socket, &pad_data, &touchpad_data, &microseconds, (const struct sockaddr*) &server_arguments->address, sizeof(struct sockaddr_in));
        if (server_arguments->enable_dsu) update_dsu(&server_arguments->socket, &microseconds, &pad_data, &touchpad_data);

        // OSTestThreadCancel();
        OSSleepTicks(OSMicrosecondsToTicks(DATA_UPDATE_RATE));
    }

    WHBLogPrint("Stopped input server thread.");

    return NULL;
}

void close_input_server(OSThread* thread) {
    run_input_server = false;
    OSJoinThread(thread, NULL);
}