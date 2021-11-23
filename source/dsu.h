#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>

#include <vpad/input.h>
#include <zlib.h>

#include "byte_swap.h"

void update_dsu(int* socket, uint64_t timestamp, VPADStatus* pad);