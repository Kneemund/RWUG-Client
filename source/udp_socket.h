#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

int init_udp_socket(const uint16_t bind_port);
void destroy_udp_socket(int* udp_socket);