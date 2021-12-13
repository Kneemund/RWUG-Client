#include "dsu.h"

#include <arpa/inet.h>
#include <string.h>
#include <zlib.h>

#include "byte_swap.h"

// This DSU implementation doesn't fully follow the specifications for the sake of efficiency.
// If this causes any issues with DSU clients, we should send information about all requested controllers
// and properly handle data requests as the specification suggests. Incoming requests are not fully validated.

// Time that can pass between data requests by the client, in nanoseconds.
// If this time is exceeded, no more data will be sent unless new data requests are received.
#define DATA_REQUEST_TIMEOUT 30000000

// Length of the incoming and outgoing packets, in bytes. Currently set to the absolute minimum.
#define INCOMING_BUFFER_SIZE 28
#define OUTGOING_BUFFER_SIZE 100

#define PROTOCOL_VERSION 1001

#define PACKET_TYPE_PROTOCOL_INFORMATION 0x100000
#define PACKET_TYPE_CONTROLLER_INFORMATION 0x100001
#define PACKET_TYPE_CONTROLLER_DATA 0x100002

void set_packet_header(uint8_t* packet, uint8_t packet_length) {
    // Magic string — DSUS if it’s message by server (you), DSUC if by client (cemuhook).
    packet[0]  = (uint8_t) 'D';
    packet[1]  = (uint8_t) 'S';
    packet[2]  = (uint8_t) 'U';
    packet[3]  = (uint8_t) 'S';

    // Protocol version used in message. Currently 1001.
    packet[4]  = (PROTOCOL_VERSION     ) & 0xFF;
    packet[5]  = (PROTOCOL_VERSION >> 8) & 0xFF;

    // Length of packet without header. Drop packet if it’s too short, truncate if it’s too long.
    packet[6]  = packet_length - 16;
    packet[7]  = 0x00;

    // CRC32
    packet[8]  = 0x00;
    packet[9]  = 0x00;
    packet[10] = 0x00;
    packet[11] = 0x00;

    // Server ID
    packet[12] = 0x01;
    packet[13] = 0x02;
    packet[14] = 0x03;
    packet[15] = 0x04;

    uint32_t checksum = bswap32u(crc32(0L, (const void*) packet, packet_length));
    memcpy(&packet[8], &checksum, sizeof(checksum));
}

uint8_t pack_protocol_information(uint8_t* packet) {
    packet[16] = (PACKET_TYPE_PROTOCOL_INFORMATION      ) & 0xFF;
    packet[17] = (PACKET_TYPE_PROTOCOL_INFORMATION >> 8 ) & 0xFF;
    packet[18] = (PACKET_TYPE_PROTOCOL_INFORMATION >> 16) & 0xFF;
    packet[19] = (PACKET_TYPE_PROTOCOL_INFORMATION >> 24) & 0xFF;

    packet[20] = (PROTOCOL_VERSION     ) & 0xFF;
    packet[21] = (PROTOCOL_VERSION >> 8) & 0xFF;

    set_packet_header(packet, 22);
    return 22;
}

uint8_t pack_controller_information(uint8_t* packet) {
    packet[16] = (PACKET_TYPE_CONTROLLER_INFORMATION      ) & 0xFF;
    packet[17] = (PACKET_TYPE_CONTROLLER_INFORMATION >> 8 ) & 0xFF;
    packet[18] = (PACKET_TYPE_CONTROLLER_INFORMATION >> 16) & 0xFF;
    packet[19] = (PACKET_TYPE_CONTROLLER_INFORMATION >> 24) & 0xFF;

    packet[20] = 0x00; // Slot you’re reporting about. Must be the same as byte value you read. (0 / GamePad)
    packet[21] = 0x02; // Slot state: 0 if not connected, 1 if reserved (?), 2 if connected. (2 / connected)
    packet[22] = 0x02; // Device model: 0 if not applicable, 1 if no or partial gyro 2 for full gyro. (2 / full gyro)
    packet[23] = 0x01; // Connection type: 0 if not applicable, 1 for USB, 2 for bluetooth. (1 / USB)

    // MAC address of device. It’s used to detect same device between launches. Zero out if not applicable. (0x000000000001)
    packet[24] = 0x01;
    packet[25] = 0x00;
    packet[26] = 0x00;
    packet[27] = 0x00;
    packet[28] = 0x00;
    packet[29] = 0x00;

    // Batery status. (0x05 / full)
    packet[30] = 0x05;

    // Termination byte.
    packet[31] = 0x00;

    set_packet_header(packet, 32);
    return 32;
}

uint8_t pack_controller_data(uint8_t* packet, uint32_t packet_count, uint64_t timestamp, float accelerometerX, float accelerometerY, float accelerometerZ, float gyroscopePitch, float gyroscopeYaw, float gyroscopeRoll, uint8_t touchpadActive, uint16_t touchpadX, uint16_t touchpadY, VPADStatus* pad) {
    packet[16] = (PACKET_TYPE_CONTROLLER_DATA      ) & 0xFF;
    packet[17] = (PACKET_TYPE_CONTROLLER_DATA >> 8 ) & 0xFF;
    packet[18] = (PACKET_TYPE_CONTROLLER_DATA >> 16) & 0xFF;
    packet[19] = (PACKET_TYPE_CONTROLLER_DATA >> 24) & 0xFF;

    packet[20] = 0x00; // Slot you’re reporting about. Must be the same as byte value you read. (0 / GamePad)
    packet[21] = 0x02; // Slot state: 0 if not connected, 1 if reserved (?), 2 if connected. (2 / connected)
    packet[22] = 0x02; // Device model: 0 if not applicable, 1 if no or partial gyro 2 for full gyro. (2 / full gyro)
    packet[23] = 0x01; // Connection type: 0 if not applicable, 1 for USB, 2 for bluetooth. (2 / USB)

    // MAC address of device. It’s used to detect same device between launches. Zero out if not applicable. (0x000000000001)
    packet[24] = 0x01;
    packet[25] = 0x00;
    packet[26] = 0x00;
    packet[27] = 0x00;
    packet[28] = 0x00;
    packet[29] = 0x00;

    // Batery status. (0x05 / full)
    packet[30] = 0x05;

    // Controller status (1 if connected, 0 if not). (0x01 / connected)
    packet[31] = 0x01;

    // Packet number (for this client).
    memcpy(&packet[32], &packet_count, sizeof(packet_count));

    uint8_t button_left  = (pad->hold & VPAD_BUTTON_LEFT)  != 0;
    uint8_t button_down  = (pad->hold & VPAD_BUTTON_DOWN)  != 0;
    uint8_t button_right = (pad->hold & VPAD_BUTTON_RIGHT) != 0;
    uint8_t button_up    = (pad->hold & VPAD_BUTTON_UP)    != 0;
    uint8_t button_y     = (pad->hold & VPAD_BUTTON_Y)     != 0;
    uint8_t button_b     = (pad->hold & VPAD_BUTTON_B)     != 0;
    uint8_t button_a     = (pad->hold & VPAD_BUTTON_A)     != 0;
    uint8_t button_x     = (pad->hold & VPAD_BUTTON_X)     != 0;
    uint8_t button_r     = (pad->hold & VPAD_BUTTON_R)     != 0;
    uint8_t button_l     = (pad->hold & VPAD_BUTTON_L)     != 0;
    uint8_t button_zr    = (pad->hold & VPAD_BUTTON_ZR)    != 0;
    uint8_t button_zl    = (pad->hold & VPAD_BUTTON_ZL)    != 0;

    packet[36] = button_left  << 7 |
                 button_down  << 6 |
                 button_right << 5 |
                 button_up    << 4 |
                 ((uint8_t) ((pad->hold & VPAD_BUTTON_PLUS)    != 0)) << 3 |
                 ((uint8_t) ((pad->hold & VPAD_BUTTON_STICK_R) != 0)) << 2 |
                 ((uint8_t) ((pad->hold & VPAD_BUTTON_STICK_L) != 0)) << 1 |
                 ((uint8_t) ((pad->hold & VPAD_BUTTON_MINUS)   != 0));

    packet[37] = button_y  << 7 |
                 button_b  << 6 |
                 button_a  << 5 |
                 button_x  << 4 |
                 button_r  << 3 |
                 button_l  << 2 |
                 button_zr << 1 |
                 button_zl;

    packet[38] = 0x00; // PS Button (unused)
    packet[39] = 0x00; // Touch Button (unused)

    // The neutral value is 127.
    packet[40] = (uint8_t) (pad->leftStick.x  * 128 + 127); // Left stick X (plus rightward)
    packet[41] = (uint8_t) (pad->leftStick.y  * 128 + 127); // Left stick Y (plus upward)
    packet[42] = (uint8_t) (pad->rightStick.x * 128 + 127); // Right stick X (plus rightward)
    packet[43] = (uint8_t) (pad->rightStick.y * 128 + 127); // Right stick Y (plus upward)

    packet[44] = button_left  * 255; // Analog D-Pad Left
    packet[45] = button_down  * 255; // Analog D-Pad Down
    packet[46] = button_right * 255; // Analog D-Pad Right
    packet[47] = button_up    * 255; // Analog D-Pad Up
    packet[48] = button_y     * 255; // Analog Y
    packet[49] = button_b     * 255; // Analog B
    packet[50] = button_a     * 255; // Analog A
    packet[51] = button_x     * 255; // Analog X
    packet[52] = button_r     * 255; // Analog R1
    packet[53] = button_l     * 255; // Analog L1
    packet[54] = button_zr    * 255; // Analog R2
    packet[55] = button_zl    * 255; // Analog L2

    // First touch.
    packet[56] = touchpadActive; // Active
    packet[57] = touchpadActive; // ID
    memcpy(&packet[58], &touchpadX, sizeof(touchpadX)); // X (2 bytes)
    memcpy(&packet[60], &touchpadY, sizeof(touchpadY)); // Y (2 bytes)

    // Second touch.
    packet[62] = 0x00; // Active
    packet[63] = 0x00; // ID
    packet[64] = 0x00; // X (2 bytes)
    packet[65] = 0x00;
    packet[66] = 0x00; // Y (2 bytes)
    packet[67] = 0x00;

    // Motion data timestamp in microseconds (8 bytes).
    memcpy(&packet[68], &timestamp, sizeof(timestamp));

    // Accelerometer data (4 bytes each).
    memcpy(&packet[76], &accelerometerX, sizeof(accelerometerX));
    memcpy(&packet[80], &accelerometerY, sizeof(accelerometerY));
    memcpy(&packet[84], &accelerometerZ, sizeof(accelerometerZ));

    // Accelerometer data (4 bytes each).
    memcpy(&packet[88], &gyroscopePitch, sizeof(gyroscopePitch));
    memcpy(&packet[92], &gyroscopeYaw, sizeof(gyroscopeYaw));
    memcpy(&packet[96], &gyroscopeRoll, sizeof(gyroscopeRoll));

    set_packet_header(packet, 100);
    return 100;
}

uint64_t last_data_requested = 0;
uint32_t outgoing_packet_count = 0;

uint8_t outgoing_packet[OUTGOING_BUFFER_SIZE];
uint8_t incoming_packet[INCOMING_BUFFER_SIZE];

struct sockaddr_in sender;
socklen_t sender_size = sizeof(sender);

void update_dsu(int* socket, uint64_t timestamp, VPADStatus* pad, VPADTouchData* touchpad) {
    // This operation is non-blocking due to MSG_DONTWAIT.
    // request_length is < 0 if recvfrom() would block.
    ssize_t request_length = recvfrom(*socket, incoming_packet, INCOMING_BUFFER_SIZE, MSG_DONTWAIT, (struct sockaddr*) &sender, &sender_size);

    // Must be longer than header (> 16) and sent by client (DSUC).
    if (request_length > 16 && strncmp((const char*) incoming_packet, "DSUC", 4) == 0) {
        switch (incoming_packet[16]) {
            // Protocol Information Request
            case 0x00: {
                uint8_t packet_size = pack_protocol_information(outgoing_packet);
                sendto(*socket, outgoing_packet, packet_size, 0, (const struct sockaddr*) &sender, sender_size);
                break;
            }

            // Controller Information Request
            case 0x01: {
                uint8_t packet_size = pack_controller_information(outgoing_packet);
                sendto(*socket, outgoing_packet, packet_size, 0, (const struct sockaddr*) &sender, sender_size);
                break;
            }

            // Controller Data Request
            case 0x02: {
                last_data_requested = timestamp;
                break;
            }
        }
    }

    if (timestamp - last_data_requested < DATA_REQUEST_TIMEOUT) {
        float accelerometerX = bswap32f(-pad->accelorometer.acc.x);
        float accelerometerY = bswap32f( pad->accelorometer.acc.y);
        float accelerometerZ = bswap32f(-pad->accelorometer.acc.z);

        float gyroscopePitch = bswap32f(-pad->gyro.x * 360.0);
        float gyroscopeYaw   = bswap32f(-pad->gyro.y * 360.0);
        float gyroscopeRoll  = bswap32f( pad->gyro.z * 360.0);

        uint8_t packet_size = pack_controller_data(
            outgoing_packet, bswap32u(outgoing_packet_count), bswap64u(timestamp),
            accelerometerX, accelerometerY, accelerometerZ,
            gyroscopePitch, gyroscopeYaw, gyroscopeRoll,
            touchpad->touched, bswap16u(touchpad->x), bswap16u(touchpad->y),
            pad
        );

        sendto(*socket, outgoing_packet, packet_size, 0, (const struct sockaddr*) &sender, sender_size);
        ++outgoing_packet_count;
    }
}
