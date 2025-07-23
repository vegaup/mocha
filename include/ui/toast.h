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
#define TOAST_PADDING 5
#define TOAST_TIMEOUT 2  // seconds

typedef struct Toast {
    Window win;
    char message[256];
    time_t created_at;
    int current_x;
    int target_x;
    int velocity;
    int alpha;
    struct Toast *next;
} Toast;

extern Toast *toasts;
extern Display *dpy;
extern Window root;
extern Colormap colormap;
extern int screen;
extern Toast *status_toast_ptr;
extern Window quote_win;

void cleanup_toasts();
void show_toast(const char *message);
void animate_toasts();
void status_toast(const char *message);
void show_quote_window(const char *quote);
void update_quote_window(const char *quote);
void destroy_quote_window();
void load_quotes(const char *filename);
const char *get_random_quote();
void draw_quote_window();

#endif  // TOAST_H