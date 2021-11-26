#include "rwug.h"

#include <string.h>
#include <jansson.h>

#define BUFFER_SIZE 512

void pack_gamepad_data(VPADStatus* pad, VPADTouchData* touchpad, char* buffer) {
    json_t* data = json_object();

    json_object_set_new_nocheck(data, "tp_touch", json_integer(touchpad->touched));
    json_object_set_new_nocheck(data, "tp_x", json_integer(touchpad->x));
    json_object_set_new_nocheck(data, "tp_y", json_integer(touchpad->y));

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
    strncpy(buffer, str, BUFFER_SIZE);

    free(str);
    json_decref(data);
}

void update_rwug(int* socket, VPADStatus* pad, VPADTouchData* touchpad, const struct sockaddr* server_address, const socklen_t server_address_size) {
    char buffer[BUFFER_SIZE];
    pack_gamepad_data(pad, touchpad, buffer);
    sendto(*socket, buffer, strlen(buffer), 0, server_address, server_address_size);
}