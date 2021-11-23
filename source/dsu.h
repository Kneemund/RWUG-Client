#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <zlib.h>

void poll_dsu(int* socket, uint64_t timestamp, float accellerometerX, float accellerometerY, float accellerometerZ, float gyroscopePit, float gyroscopeYaw, float gyroscopeRol);