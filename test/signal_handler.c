#include "signal_handler.h"

#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <SDL3/SDL.h>

sigset_t g_sigset;
bool g_done = false;

int signal_handler_thread(void* data) {
    (void)data;
    int sig;
    sigwait(&g_sigset, &sig);
    g_done = true;
    return 0;
}

void signal_handler_run(void) {
    sigemptyset(&g_sigset);
    sigaddset(&g_sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &g_sigset, NULL);
    SDL_Thread* thread = SDL_CreateThread(signal_handler_thread, "signal", NULL);
    (void)thread;
}

void signal_handler_quit(void) {
    g_done = true;
}

bool signal_handler_should_quit(void) {
    return g_done;
}
