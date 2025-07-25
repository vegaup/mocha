#define _GNU_SOURCE
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "ui/toast.h"

/**
 * A simple util to print out with Mocha prefix
 */
void mocha_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("\033[1;30m[ \033[0m\033[1;35mMocha\033[0m\033[1;30m ]\033[0m ");
    vprintf(fmt, args);
    printf("\n");
    fflush(stdout);
    va_end(args);
}

/**
 * A simple util to display errors
 */
void mocha_error(FILE *stream, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stream,
            "\033[1;30m[ \033[0m\033[1;35mMocha\033[0m\033[1;30m ]\033[0m ");
    vfprintf(stream, fmt, args);
    fprintf(stream, "\n");
    va_end(args);
}

/**
 * Simple util to use XGrabKey
 */
void grabKey(KeySym sym, unsigned int mod) {
    KeyCode code = XKeysymToKeycode(dpy, sym);
    if(code == 0) {
        mocha_log("Could not get keycode for keysym");
        return;
    }

    XGrabKey(dpy, code, mod, root, True, GrabModeAsync, GrabModeAsync);
    XSync(dpy, False);
}

/**
 * Simple panic util
 */
void panic(char *msg) {
    mocha_log(msg);
    exit(EXIT_FAILURE);
}

/**
 * Improved X error handler with rate limiting
 */
int handleXError(Display *dpy, XErrorEvent *e) {
    char error_text[1024];
    XGetErrorText(dpy, e->error_code, error_text, sizeof(error_text));
    mocha_log(
        "X Error: %s (code: %d, request code: %d, minor code: %d, resource id: "
        "%lu)",
        error_text, e->error_code, e->request_code, e->minor_code,
        e->resourceid);
    return 0;
}

/**
 * Util to run a shell command and capture its output as a string
 */
int run_command(const char *cmd, char *buf, size_t buflen) {
    FILE *fp = popen(cmd, "r");
    if(!fp) return -1;
    size_t n = fread(buf, 1, buflen - 1, fp);
    buf[n] = '\0';
    pclose(fp);
    return 0;
}

void mocha_shutdown() {
    mocha_log("Mocha is shutting down...");
    cleanup_toasts();
    XCloseDisplay(dpy);
    exit(0);
}