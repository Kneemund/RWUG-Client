#include <stdint.h>

typedef struct {
    const char* ip_address;
    uint8_t mode;
} configuration;

void get_configuration_path(char* path);
configuration load_configuration(const char* path);
void save_configuration(const char* path, const char* ip_address, const uint8_t mode);