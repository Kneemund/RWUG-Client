typedef struct {
    const char* ip_address;
} configuration;

void get_configuration_path(char* path);
configuration load_configuration(const char* path);
void save_configuration(const char* path, const char* ip_address);