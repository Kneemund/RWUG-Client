#include "rwug.h"

#include <string.h>

#include "byte_swap.h"

#define RWUG_PLAY 0x01
#define RWUG_STOP 0x02

#define RWUG_OUT_SIZE 58
#define RWUG_IN_SIZE 4

#define FORCE_FEEDBACK_MAX_PATTERN_SIZE 120

void pack_gamepad_data(VPADStatus* pad, VPADTouchData* touchpad, uint8_t* packet, uint64_t* microseconds) {
    float accelerometerX = bswap32f(-pad->accelorometer.acc.x);
    float accelerometerY = bswap32f( pad->accelorometer.acc.y);
    float accelerometerZ = bswap32f(-pad->accelorometer.acc.z);

    // Accelerometer data (4 bytes each).
    memcpy(&packet[0], &accelerometerX, sizeof(accelerometerX));
    memcpy(&packet[4], &accelerometerY, sizeof(accelerometerY));
    memcpy(&packet[8], &accelerometerZ, sizeof(accelerometerZ));

    float gyroscopePitch = bswap32f(-pad->gyro.x * 360.0);
    float gyroscopeYaw   = bswap32f(-pad->gyro.y * 360.0);
    float gyroscopeRoll  = bswap32f( pad->gyro.z * 360.0);

    // Gyroscope data (4 bytes each).
    memcpy(&packet[12], &gyroscopePitch, sizeof(gyroscopePitch));
    memcpy(&packet[16], &gyroscopeYaw,   sizeof(gyroscopeYaw));
    memcpy(&packet[20], &gyroscopeRoll,  sizeof(gyroscopeRoll));

    uint16_t touchpadX = bswap16u(touchpad->x);
    uint16_t touchpadY = bswap16u(touchpad->y);

    // First touch.
    packet[24] = touchpad->touched; // Active
    packet[25] = touchpad->touched; // ID
    memcpy(&packet[26], &touchpadX, sizeof(touchpadX)); // X (2 bytes)
    memcpy(&packet[28], &touchpadY, sizeof(touchpadY)); // Y (2 bytes)

    uint64_t timestamp = bswap64u(*microseconds);

    // Motion data timestamp in microseconds (8 bytes).
    memcpy(&packet[30], &timestamp, sizeof(timestamp));

    uint32_t hold = bswap32u(pad->hold);

    // Held button bitfield (4 bytes).
    memcpy(&packet[38], &hold, sizeof(hold));

    float stickLX = bswap32f(pad->leftStick.x);
    float stickLY = bswap32f(pad->leftStick.y);
    float stickRX = bswap32f(pad->rightStick.x);
    float stickRY = bswap32f(pad->rightStick.y);

    // Stick values (4 bytes).
    memcpy(&packet[42], &stickLX, sizeof(stickLX));
    memcpy(&packet[46], &stickLY, sizeof(stickLX));
    memcpy(&packet[50], &stickRX, sizeof(stickLX));
    memcpy(&packet[54], &stickRY, sizeof(stickLX));
}

static uint16_t force_feedback_length = 0;

void handle_force_feedback(int* socket) {
    /*
        1 - packet type
        2 - strength
        3 - length in ms (2 bytes)
        4 - length in ms (2 bytes)
    */

    uint8_t incoming_packet[RWUG_IN_SIZE];
    while (recv(*socket, incoming_packet, RWUG_IN_SIZE, MSG_DONTWAIT) > 0) {
        if (incoming_packet[0] == RWUG_STOP || incoming_packet[1] < 10) {
            // stop packet or very low strength (3% and below)
            force_feedback_length = 0;

            VPADStopMotor(VPAD_CHAN_0);
        } else if (incoming_packet[0] == RWUG_PLAY) {
            // play packet and normal strength
            memcpy(&force_feedback_length, &incoming_packet[2], sizeof(uint16_t));
            force_feedback_length = bswap16u(force_feedback_length) * (120.0 / 1000.0); // uinput length in ms, VPAD length of 120 is about 1000ms

            VPADStopMotor(VPAD_CHAN_0);
        }
    }

    if (VPADBASEGetMotorOnRemainingCount(VPAD_CHAN_0) == 0 && force_feedback_length > 0) {
        // motor not enabled and force feedback queued

        uint8_t step = force_feedback_length < FORCE_FEEDBACK_MAX_PATTERN_SIZE ? force_feedback_length : FORCE_FEEDBACK_MAX_PATTERN_SIZE;

        uint8_t pattern[step];
        memset(&pattern[0], incoming_packet[1], step);

        VPADControlMotor(VPAD_CHAN_0, pattern, step);
        force_feedback_length -= step;
    }
}

void update_rwug(int* socket, VPADStatus* pad, VPADTouchData* touchpad, uint64_t* microseconds, const struct sockaddr* server_address, const socklen_t server_address_size) {
    uint8_t outgoing_packet[RWUG_OUT_SIZE];
    pack_gamepad_data(pad, touchpad, outgoing_packet, microseconds);
    sendto(*socket, outgoing_packet, RWUG_OUT_SIZE, 0, server_address, server_address_size);

    handle_force_feedback(socket);
}