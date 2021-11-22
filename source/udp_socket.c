#include "udp_socket.h"

int init_udp_socket(const char* ip_string, uint16_t port) {
    int udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_socket < 0) return -1;

    struct sockaddr_in connect_addr;
    memset(&connect_addr, 0, sizeof(connect_addr));

    connect_addr.sin_family = AF_INET;
    connect_addr.sin_port = htons(port);
    inet_aton(ip_string, &connect_addr.sin_addr);

    if (connect(udp_socket, (struct sockaddr*) &connect_addr, sizeof(connect_addr)) < 0) {
        close(udp_socket);
        return -1;
    }

    return udp_socket;
}

void destroy_udp_socket(int* udp_socket) {
    if (*udp_socket >= 0) {
        close(*udp_socket);
        *udp_socket = -1;
    }
}
