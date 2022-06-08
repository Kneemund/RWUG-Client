#include <coreinit/thread.h>
#include <arpa/inet.h>

struct InputServerArguments {
    int socket;
    struct sockaddr_in address;

    const bool enable_rwug;
    const bool enable_dsu;
};

void* input_server(void* args);
void close_input_server(OSThread* thread);