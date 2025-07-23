#define _GNU_SOURCE
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
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
 * Simple error handling util
 */
int handleXError(Display *_, XErrorEvent *error) {
    switch(error->error_code) {
        case BadAccess:
            mocha_error(
                stderr,
                "[%d]=BadAccess: Is another window manager already running?\n",
                error->error_code);
            exit(1);  // fatal

        case BadMatch:
            mocha_error(stderr,
                        "[%d]=BadMatch: Parameter mismatch. This may cause "
                        "visual glitches.\n",
                        error->error_code);
            break;

        case BadDrawable:
            mocha_error(
                stderr,
                "[%d]=BadDrawable: Invalid drawable. Likely transient.\n",
                error->error_code);
            break;

        case BadAlloc:
            mocha_error(stderr, "[%d]=BadAlloc: Insufficient resources.\n",
                        error->error_code);
            break;

        case BadWindow:
            mocha_error(stderr, "[%d]=BadWindow: Invalid window. Skipping...\n",
                        error->error_code);
            break;

        case BadValue:
            mocha_error(
                stderr,
                "[%d]=BadValue: Invalid value passed to X11. Recovering...\n",
                error->error_code);
            break;

        case BadAtom:
            mocha_error(stderr, "[%d]=BadAtom: Invalid atom. Skipping...\n",
                        error->error_code);
            break;

        case BadColor:
            mocha_error(stderr, "[%d]=BadColor: Invalid color. Skipping...\n",
                        error->error_code);
            break;

        case BadCursor:
            mocha_error(stderr, "[%d]=BadCursor: Invalid cursor. Skipping...\n",
                        error->error_code);
            break;

        case BadFont:
            mocha_error(stderr, "[%d]=BadFont: Invalid font. Skipping...\n",
                        error->error_code);
            break;

        case BadGC:
            mocha_error(stderr,
                        "[%d]=BadGC: Invalid graphics context. Skipping...\n",
                        error->error_code);
            break;

        case BadPixmap:
            mocha_error(stderr, "[%d]=BadPixmap: Invalid pixmap. Skipping...\n",
                        error->error_code);
            break;

        case BadLength:
            mocha_error(stderr, "[%d]=BadLength: Incorrect request length.\n",
                        error->error_code);
            break;

        case BadImplementation:
            mocha_error(stderr,
                        "[%d]=BadImplementation: Unsupported request.\n",
                        error->error_code);
            break;

        default:
            mocha_error(stderr, "[%d] Unknown X error occurred. Ignoring.\n",
                        error->error_code);
            break;
    }

    return 0;
}

/**
 * Util to run a shell command and capture its output as a string
 */
int run_command(const char *cmd, char *buf, size_t buflen) {
    FILE *fp = popen(cmd, "r");
    if (!fp) return -1;
    size_t n = fread(buf, 1, buflen - 1, fp);
    buf[n] = '\0';
    pclose(fp);
    return 0;
}

void mocha_shutdown() {
    cleanup_toasts();
    XCloseDisplay(dpy);
    exit(0);
}