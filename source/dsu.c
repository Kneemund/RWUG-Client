#include "dsu.h"

// This DSU implementation doesn't fully follow the specifications for the sake of efficiency.
// If this causes any issues with DSU clients, we should send information about all requested controllers
// and properly handle data requests as the specification suggests.

#define DATA_REQUEST_TIMEOUT 30000000

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
    packet[23] = 0x02; // Connection type: 0 if not applicable, 1 for USB, 2 for bluetooth. (2 / bluetooth)

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

uint8_t pack_empty_controller_information(uint8_t* packet, uint8_t slot) {
    packet[16] = (PACKET_TYPE_CONTROLLER_INFORMATION      ) & 0xFF;
    packet[17] = (PACKET_TYPE_CONTROLLER_INFORMATION >> 8 ) & 0xFF;
    packet[18] = (PACKET_TYPE_CONTROLLER_INFORMATION >> 16) & 0xFF;
    packet[19] = (PACKET_TYPE_CONTROLLER_INFORMATION >> 24) & 0xFF;

    packet[20] = slot; // Slot you’re reporting about. Must be the same as byte value you read.
    packet[21] = 0x00; // Slot state: 0 if not connected, 1 if reserved (?), 2 if connected. (0 / not connected)
    packet[22] = 0x00; // Device model: 0 if not applicable, 1 if no or partial gyro 2 for full gyro. (0 / not applicable)
    packet[23] = 0x00; // Connection type: 0 if not applicable, 1 for USB, 2 for bluetooth. (0 / not applicable)

    // MAC address of device. It’s used to detect same device between launches. Zero out if not applicable. (not applicable)
    packet[24] = 0x00;
    packet[25] = 0x00;
    packet[26] = 0x00;
    packet[27] = 0x00;
    packet[28] = 0x00;
    packet[29] = 0x00;

    // Batery status. (0x00 / not applicable)
    packet[30] = 0x00;

    // Termination byte.
    packet[31] = 0x00;

    set_packet_header(packet, 32);
    return 32;
}

uint8_t pack_controller_data(uint8_t* packet, uint32_t packet_count, uint64_t timestamp, float accelerometerX, float accelerometerY, float accelerometerZ, float gyroscopePit, float gyroscopeYaw, float gyroscopeRol) {
    packet[16] = (PACKET_TYPE_CONTROLLER_DATA      ) & 0xFF;
    packet[17] = (PACKET_TYPE_CONTROLLER_DATA >> 8 ) & 0xFF;
    packet[18] = (PACKET_TYPE_CONTROLLER_DATA >> 16) & 0xFF;
    packet[19] = (PACKET_TYPE_CONTROLLER_DATA >> 24) & 0xFF;

    packet[20] = 0x00; // Slot you’re reporting about. Must be the same as byte value you read. (0 / GamePad)
    packet[21] = 0x02; // Slot state: 0 if not connected, 1 if reserved (?), 2 if connected. (2 / connected)
    packet[22] = 0x02; // Device model: 0 if not applicable, 1 if no or partial gyro 2 for full gyro. (2 / full gyro)
    packet[23] = 0x02; // Connection type: 0 if not applicable, 1 for USB, 2 for bluetooth. (2 / bluetooth)

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

    packet[36] = 0x00; // D-Pad Left, D-Pad Down, D-Pad Right, D-Pad Up, Options (?), R3, L3, Share (?)
    packet[37] = 0x00; // Y, B, A, X, R1, L1, R2, L2
    packet[38] = 0x00; // PS Button (unused)
    packet[39] = 0x00; // Touch Button (unused)
    packet[40] = 0x00; // Left stick X (plus rightward)
    packet[41] = 0x00; // Left stick Y (plus upward)
    packet[42] = 0x00; // Right stick X (plus rightward)
    packet[43] = 0x00; // Right stick Y (plus upward)
    packet[44] = 0x00; // Analog D-Pad Left
    packet[45] = 0x00; // Analog D-Pad Down
    packet[46] = 0x00; // Analog D-Pad Right
    packet[47] = 0x00; // Analog D-Pad Up
    packet[48] = 0x00; // Analog Y
    packet[49] = 0x00; // Analog B
    packet[50] = 0x00; // Analog A
    packet[51] = 0x00; // Analog X
    packet[52] = 0x00; // Analog R1
    packet[53] = 0x00; // Analog L1
    packet[54] = 0x00; // Analog R2
    packet[55] = 0x00; // Analog L2

    // First touch.
    packet[56] = 0x00; // Active
    packet[57] = 0x00; // ID
    packet[58] = 0x00; // X (2 bytes)
    packet[59] = 0x00;
    packet[60] = 0x00; // Y (2 bytes)
    packet[61] = 0x00;

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
    memcpy(&packet[88], &gyroscopePit, sizeof(gyroscopePit));
    memcpy(&packet[92], &gyroscopeYaw, sizeof(gyroscopeYaw));
    memcpy(&packet[96], &gyroscopeRol, sizeof(gyroscopeRol));

    set_packet_header(packet, 100);
    return 100;
}

uint8_t incoming_packet[28];
uint8_t outgoing_packet[100];

uint64_t last_data_requested = 0;
uint32_t outgoing_packet_count = 0;

struct timeval timeout = { 0, 10000 };

struct sockaddr_in sender;
socklen_t sender_size = sizeof(sender);

void update_dsu(int* socket, uint64_t timestamp, VPADStatus* pad) {
    fd_set read_fd;
    FD_ZERO(&read_fd);
    FD_SET(*socket, &read_fd);

    if (select(*socket + 1, &read_fd, NULL, NULL, &timeout) > 0) {
        ssize_t request_length = recvfrom(*socket, incoming_packet, 28, 0x100, (struct sockaddr*) &sender, &sender_size);

        if (request_length > 0) {
            switch (incoming_packet[16]) {
                // Protocol Information Request
                case 0x00:
                    uint8_t packet_size = pack_protocol_information(outgoing_packet);
                    sendto(*socket, outgoing_packet, packet_size, 0, (const struct sockaddr*) &sender, sender_size);
                break;

                // Controller Information Request
                case 0x01:
                    // TODO: do we have to send information about controllers that don't exist?
                    uint8_t slot_count = incoming_packet[20];
                    for (uint8_t i = 0; i < slot_count; ++i) {
                        uint8_t slot = incoming_packet[24 + i];

                        uint8_t packet_size = slot == 0 ? pack_controller_information(outgoing_packet) : pack_empty_controller_information(outgoing_packet, slot);
                        sendto(*socket, outgoing_packet, packet_size, 0, (const struct sockaddr*) &sender, sender_size);
                    }
                break;

                // Controller Data Request
                case 0x02:
                    last_data_requested = timestamp;

                    // all controllers
                    // if (incoming_packet[20] == 0x00) {

                    // }

                    // controller by slot
                    // if (incoming_packet[20] & 0x01) {
                    //     uint8_t slot = incoming_packet[21];

                    // }

                    // controller by mac
                    // if (incoming_packet[20] & 0x02) {

                    // }
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

        uint8_t packet_size = pack_controller_data(outgoing_packet, bswap32u(outgoing_packet_count), timestamp, accelerometerX, accelerometerY, accelerometerZ, gyroscopePitch, gyroscopeYaw, gyroscopeRoll);
        sendto(*socket, outgoing_packet, packet_size, 0, (const struct sockaddr*) &sender, sender_size);
        ++outgoing_packet_count;
    }
}
