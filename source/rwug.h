#include <vpad/input.h>
#include <arpa/inet.h>

void update_rwug(int* socket, VPADStatus* pad, VPADTouchData* touchpad, const struct sockaddr* server_address, const socklen_t server_address_size);