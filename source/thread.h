#include <coreinit/thread.h>

int create_thread(OSThread **outThread, void* (*entryPoint) (void*), void *entryArgs);