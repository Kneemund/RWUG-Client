#include <sys/select.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include <coreinit/dynload.h>

void dsu_init();
void poll_dsu(int* socket, uint64_t timestamp, float accellerometerX, float accellerometerY, float accellerometerZ, float gyroscopePit, float gyroscopeYaw, float gyroscopeRol);