#include "dsu.h"

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
    packet[6]  = packet_length;
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
}

uint8_t encode_protocol_information(uint8_t* output) {
    set_packet_header(output, 6);

    output[16] = (PACKET_TYPE_PROTOCOL_INFORMATION      ) & 0xFF;
    output[17] = (PACKET_TYPE_PROTOCOL_INFORMATION >> 8 ) & 0xFF;
    output[18] = (PACKET_TYPE_PROTOCOL_INFORMATION >> 16) & 0xFF;
    output[19] = (PACKET_TYPE_PROTOCOL_INFORMATION >> 24) & 0xFF;

    output[20] = (PROTOCOL_VERSION     ) & 0xFF;
    output[21] = (PROTOCOL_VERSION >> 8) & 0xFF;

    uint32_t checksum = crc32(0L, (const void*) output, 22);
    memcpy(&output[8], &checksum, sizeof(checksum));

    return 22;
}

uint8_t encode_controller_information(uint8_t* output) {
    set_packet_header(output, 16);

    output[16] = (PACKET_TYPE_CONTROLLER_INFORMATION      ) & 0xFF;
    output[17] = (PACKET_TYPE_CONTROLLER_INFORMATION >> 8 ) & 0xFF;
    output[18] = (PACKET_TYPE_CONTROLLER_INFORMATION >> 16) & 0xFF;
    output[19] = (PACKET_TYPE_CONTROLLER_INFORMATION >> 24) & 0xFF;

    output[20] = 0x00; // Slot you’re reporting about. Must be the same as byte value you read. (0 / GamePad)
    output[21] = 0x02; // Slot state: 0 if not connected, 1 if reserved (?), 2 if connected. (2 / connected)
    output[22] = 0x02; // Device model: 0 if not applicable, 1 if no or partial gyro 2 for full gyro. (2 / full gyro)
    output[23] = 0x02; // Connection type: 0 if not applicable, 1 for USB, 2 for bluetooth. (2 / bluetooth)

    // MAC address of device. It’s used to detect same device between launches. Zero out if not applicable. (0x000000000001)
    output[24] = 0x01;
    output[25] = 0x00;
    output[26] = 0x00;
    output[27] = 0x00;
    output[28] = 0x00;
    output[29] = 0x00;

    // Batery status. (0x05 / full)
    output[30] = 0x05;

    // Termination byte.
    output[31] = 0x00;


    uint32_t checksum = crc32(0L, (const void*) output, 32);
    memcpy(&output[8], &checksum, sizeof(checksum));

    return 32;
}

uint8_t encode_empty_controller_information(uint8_t* output, uint8_t slot) {
    set_packet_header(output, 16);

    output[16] = (PACKET_TYPE_CONTROLLER_INFORMATION      ) & 0xFF;
    output[17] = (PACKET_TYPE_CONTROLLER_INFORMATION >> 8 ) & 0xFF;
    output[18] = (PACKET_TYPE_CONTROLLER_INFORMATION >> 16) & 0xFF;
    output[19] = (PACKET_TYPE_CONTROLLER_INFORMATION >> 24) & 0xFF;

    output[20] = slot; // Slot you’re reporting about. Must be the same as byte value you read. (0 / GamePad)
    output[21] = 0x00; // Slot state: 0 if not connected, 1 if reserved (?), 2 if connected. (2 / connected)
    output[22] = 0x00; // Device model: 0 if not applicable, 1 if no or partial gyro 2 for full gyro. (2 / full gyro)
    output[23] = 0x00; // Connection type: 0 if not applicable, 1 for USB, 2 for bluetooth. (2 / bluetooth)

    // MAC address of device. It’s used to detect same device between launches. Zero out if not applicable. (0x000000000001)
    output[24] = 0x00;
    output[25] = 0x00;
    output[26] = 0x00;
    output[27] = 0x00;
    output[28] = 0x00;
    output[29] = 0x00;

    // Batery status. (0x05 / full)
    output[30] = 0x00;

    // Termination byte.
    output[31] = 0x00;


    uint32_t checksum = crc32(0L, (const void*) output, 32);
    memcpy(&output[8], &checksum, sizeof(checksum));

    return 32;
}

uint8_t encode_empty_controller_data(uint8_t* output, uint32_t packet_count, uint8_t slot) {
    set_packet_header(output, 84);

    output[16] = (PACKET_TYPE_CONTROLLER_DATA      ) & 0xFF;
    output[17] = (PACKET_TYPE_CONTROLLER_DATA >> 8 ) & 0xFF;
    output[18] = (PACKET_TYPE_CONTROLLER_DATA >> 16) & 0xFF;
    output[19] = (PACKET_TYPE_CONTROLLER_DATA >> 24) & 0xFF;

    output[20] = slot; // Slot you’re reporting about. Must be the same as byte value you read.
    output[21] = 0x00; // Slot state: 0 if not connected, 1 if reserved (?), 2 if connected. (0 / not connected)
    output[22] = 0x00; // Device model: 0 if not applicable, 1 if no or partial gyro 2 for full gyro. (2 / full gyro)
    output[23] = 0x00; // Connection type: 0 if not applicable, 1 for USB, 2 for bluetooth. (2 / bluetooth)

    // MAC address of device. It’s used to detect same device between launches. Zero out if not applicable. (not applicable)
    output[24] = 0x00;
    output[25] = 0x00;
    output[26] = 0x00;
    output[27] = 0x00;
    output[28] = 0x00;
    output[29] = 0x00;

    // Batery status. (0x05 / not applicable)
    output[30] = 0x00;

    // Controller status (1 if connected, 0 if not). (0x00 / not connected)
    output[31] = 0x00;

    // Packet number (for this client).
    memcpy(&output[32], &packet_count, sizeof(packet_count));

    // Fill packet with 0x00.
    memset(&output[36], 0, 64);

    uint32_t checksum = crc32(0L, (const void*) output, 100);
    memcpy(&output[8], &checksum, sizeof(checksum));

    return 100;
}


uint8_t encode_controller_data(uint8_t* output, uint32_t packet_count, uint64_t timestamp, float accellerometerX, float accellerometerY, float accellerometerZ, float gyroscopePit, float gyroscopeYaw, float gyroscopeRol) {
    set_packet_header(output, 84);

    output[16] = (PACKET_TYPE_CONTROLLER_DATA      ) & 0xFF;
    output[17] = (PACKET_TYPE_CONTROLLER_DATA >> 8 ) & 0xFF;
    output[18] = (PACKET_TYPE_CONTROLLER_DATA >> 16) & 0xFF;
    output[19] = (PACKET_TYPE_CONTROLLER_DATA >> 24) & 0xFF;

    output[20] = 0x00; // Slot you’re reporting about. Must be the same as byte value you read. (0 / GamePad)
    output[21] = 0x02; // Slot state: 0 if not connected, 1 if reserved (?), 2 if connected. (2 / connected)
    output[22] = 0x02; // Device model: 0 if not applicable, 1 if no or partial gyro 2 for full gyro. (2 / full gyro)
    output[23] = 0x02; // Connection type: 0 if not applicable, 1 for USB, 2 for bluetooth. (2 / bluetooth)

    // MAC address of device. It’s used to detect same device between launches. Zero out if not applicable. (0x000000000001)
    output[24] = 0x01;
    output[25] = 0x00;
    output[26] = 0x00;
    output[27] = 0x00;
    output[28] = 0x00;
    output[29] = 0x00;

    // Batery status. (0x05 / full)
    output[30] = 0x05;

    // Controller status (1 if connected, 0 if not). (0x01 / connected)
    output[31] = 0x01;

    // Packet number (for this client).
    memcpy(&output[32], &packet_count, sizeof(packet_count));

    output[36] = 0x00; // D-Pad Left, D-Pad Down, D-Pad Right, D-Pad Up, Options (?), R3, L3, Share (?)
    output[37] = 0x00; // Y, B, A, X, R1, L1, R2, L2
    output[38] = 0x00; // PS Button (unused)
    output[39] = 0x00; // Touch Button (unused)
    output[40] = 0x00; // Left stick X (plus rightward)
    output[41] = 0x00; // Left stick Y (plus upward)
    output[42] = 0x00; // Right stick X (plus rightward)
    output[43] = 0x00; // Right stick Y (plus upward)
    output[44] = 0x00; // Analog D-Pad Left
    output[45] = 0x00; // Analog D-Pad Down
    output[46] = 0x00; // Analog D-Pad Right
    output[47] = 0x00; // Analog D-Pad Up
    output[48] = 0x00; // Analog Y
    output[49] = 0x00; // Analog B
    output[50] = 0x00; // Analog A
    output[51] = 0x00; // Analog X
    output[52] = 0x00; // Analog R1
    output[53] = 0x00; // Analog L1
    output[54] = 0x00; // Analog R2
    output[55] = 0x00; // Analog L2

    // First touch.
    output[56] = 0x00; // Active
    output[57] = 0x00; // ID
    output[58] = 0x00; // X (2 bytes)
    output[59] = 0x00;
    output[60] = 0x00; // Y (2 bytes)
    output[61] = 0x00;

    // Second touch.
    output[62] = 0x00; // Active
    output[63] = 0x00; // ID
    output[64] = 0x00; // X (2 bytes)
    output[65] = 0x00;
    output[66] = 0x00; // Y (2 bytes)
    output[67] = 0x00;

    // Motion data timestamp in microseconds (8 bytes).
    memcpy(&output[68], &timestamp, sizeof(timestamp));

    // Accelerometer data (4 bytes each).
    memcpy(&output[76], &accellerometerX, sizeof(accellerometerX));
    memcpy(&output[80], &accellerometerY, sizeof(accellerometerY));
    memcpy(&output[84], &accellerometerZ, sizeof(accellerometerZ));

    // Accelerometer data (4 bytes each).
    memcpy(&output[88], &gyroscopePit, sizeof(gyroscopePit));
    memcpy(&output[92], &gyroscopeYaw, sizeof(gyroscopeYaw));
    memcpy(&output[96], &gyroscopeRol, sizeof(gyroscopeRol));

    uint32_t checksum = crc32(0L, (const void*) output, 100);
    memcpy(&output[8], &checksum, sizeof(checksum));

    return 100;
}

uint8_t incoming_packet[28];
uint8_t packet[100];
uint8_t packet_size;
uint8_t packet_number = 0;

uint8_t requested_slots[4];

fd_set read_fd;
struct timeval timeout = { 0, 10000 };

struct sockaddr_in sender;
socklen_t sender_size = sizeof(sender);

void poll_dsu(int* socket, uint64_t timestamp, float accellerometerX, float accellerometerY, float accellerometerZ, float gyroscopePit, float gyroscopeYaw, float gyroscopeRol) {
    FD_ZERO(&read_fd);
    FD_SET(*socket, &read_fd);

    if (select(*socket + 1, &read_fd, NULL, NULL, &timeout) > 0) {
        ssize_t request_length = recvfrom(*socket, incoming_packet, 28, 0x100, (struct sockaddr*) &sender, &sender_size);
        if (request_length > 0) {
            // LSB of packet type.
            switch (incoming_packet[16]) {
                // Protocol Information Request
                case 0x00:
                    packet_size = encode_protocol_information(&packet[0]);
                    sendto(*socket, packet, packet_size, 0, (const struct sockaddr*) &sender, sender_size);
                break;

                // Controller Information Request
                case 0x01:
                    uint8_t slot_count = incoming_packet[20];
                    for (uint8_t i = 0; i < slot_count; ++i) {
                        uint8_t slot = incoming_packet[24 + i];

                        if (slot == 0) packet_size = encode_controller_information(&packet[0]);
                        else packet_size = encode_empty_controller_information(&packet[0], slot);
                        sendto(*socket, packet, packet_size, 0, (const struct sockaddr*) &sender, sender_size);
                    }

                break;

                // Controller Data Request
                case 0x02:
                    // all controllers
                    if (incoming_packet[20] == 0x00) {
                        requested_slots[0] = 1;
                        requested_slots[1] = 1;
                        requested_slots[2] = 1;
                        requested_slots[3] = 1;
                    }

                    // controller by slot
                    if (incoming_packet[20] & 0x01) {
                        if (incoming_packet[21] > 4) return;
                        requested_slots[incoming_packet[21]] = 1;
                    }

                    // controller by mac
                    // if (incoming_packet[20] & 0x02) {

                    // }
                break;
            }
        }
    }

    for (uint8_t i = 0; i < 4; ++i) {
        if (requested_slots[i] != 1) continue;

        if (i == 0) packet_size = encode_controller_data(&packet[0], packet_number, timestamp, accellerometerX, accellerometerY, accellerometerZ, gyroscopePit, gyroscopeYaw, gyroscopeRol);
        else packet_size = encode_empty_controller_data(&packet[0], packet_number, i);

        sendto(*socket, packet, packet_size, 0, (const struct sockaddr*) &sender, sender_size);
        ++packet_number;
    }
}
