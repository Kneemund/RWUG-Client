#include "thread.h"

#include <string.h>
#include <malloc.h>

static void thread_deallocator(OSThread *thread, void* stack) {
    free(stack);
    free(thread);
}

int create_thread(OSThread** outThread, void* (*entryPoint) (void*), void *entryArgs) {
    OSThread* thread = (OSThread*) memalign(16, sizeof(OSThread));
    const int stack_size = 4 * 1024 * 1024;
    uint8_t* stack = (uint8_t*) memalign(16, stack_size);
    memset(thread, 0, sizeof(OSThread));

    if (!OSCreateThread(thread, (OSThreadEntryPointFn) entryPoint, (int) entryArgs, NULL, stack + stack_size, stack_size, 16, OS_THREAD_ATTRIB_AFFINITY_ANY)) {
        free(thread);
        free(stack);
        return -1;
    }

    *outThread = thread;
    OSSetThreadDeallocator(thread, thread_deallocator);
    OSResumeThread(thread);

    return 0;
}