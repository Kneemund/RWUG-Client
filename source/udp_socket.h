#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

int init_udp_socket(const char* ip_string, uint16_t port);
void destroy_udp_socket(int* udp_socket);
