#ifndef TOAST_H
#define TOAST_H

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "main.h"
#include "util/client.h"

#define MAX_TOASTS 5
#define TOAST_WIDTH 300
#define TOAST_HEIGHT 50
#define TOAST_PADDING 10
#define TOAST_TIMEOUT 5  // seconds

typedef struct Toast {
    Window win;
    char message[256];
    time_t created_at;
    struct Toast *next;
} Toast;

extern Toast *toasts;
extern Display *dpy;
extern Window root;
extern Colormap colormap;
extern int screen;

void cleanup_toasts();
void show_toast(const char *message);

#endif  // TOAST_H