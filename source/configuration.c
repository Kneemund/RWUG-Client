#include "configuration.h"

#include <whb/sdcard.h>
#include <string.h>
#include <ini.h>

static int handler(void* out, const char* section, const char* name, const char* value) {
    configuration* config = (configuration*) out;

    if (strcmp(section, "general") == 0) {
        if (strcmp(name, "ip_address") == 0) {
            config->ip_address = strdup(value);
        } else {
            return 0;
        }
    } else {
        return 0;
    }

    return 1;
}

void get_configuration_path(char* path) {
    sprintf(path, "%s/wiiu/apps/RWUG/configuration.ini", WHBGetSdCardMountPath());
}

configuration load_configuration(const char* path) {
    configuration config = { NULL };
    ini_parse(path, handler, &config);

    return config;
}

void save_configuration(const char* path, const char* ip_address) {
    FILE* file = fopen(path, "w");
    if (file != NULL) {
        fprintf(file, "[general]\nip_address=%s\n\n", ip_address);
        fclose(file);
    }
}