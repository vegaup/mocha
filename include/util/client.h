#ifndef CLIENT_H
#define CLIENT_H

#include <X11/Xlib.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>
#include <stdbool.h>

#include "main.h"
#include "util/app.h"

/* Maximum amount of clients to allow at once */
#define MAX_CLIENTS 256
/* Maximum number of dock icons */
#define MAX_DOCK_ICONS 16
/* Macro to iterate through all clients */
#define mocha_for_each_client(cvar, wvar)                \
    for(int __i = 0; __i < num_managed_clients; __i++) { \
        Window wvar = managed_clients[__i];              \
        ClientState *cvar = mocha_get_client_state(wvar);
#define mocha_for_each_client_end }

/* Struct to keep track of client state */
typedef struct {
    int is_fullscreen;
    int is_minimized;
    int saved_x, saved_y, saved_w, saved_h;
} ClientState;

/* Struct for dock icons */
typedef struct {
    char name[256];
    char command[256];
    bool is_running;
    Window window;
    AppInfo *app;
    int x, y;
} DockIcon;

/* All client states */
extern ClientState client_states[MAX_CLIENTS];
/* 1 client state */
extern ClientState *mocha_get_client_state(Window w);
/* All windows being managed */
extern Window managed_clients[MAX_CLIENTS];
/* Number of managed clients */
extern int num_managed_clients;
/* Dock icons */
extern DockIcon dock_icons[MAX_DOCK_ICONS];
/* Number of dock icons */
extern int num_dock_icons;

void mocha_add_managed_client(Window w);
void mocha_remove_managed_client(Window w);
void mocha_tile_clients(int taskbar_height);
char *mocha_get_client_name(Window w);
int is_dialog(Window w);
void mocha_init_dock();
void mocha_add_dock_icon(const char *name, const char *command);
void mocha_draw_dock(Window dock_win);
void mocha_handle_dock_click(int x, int y);
void mocha_update_dock_icons();
void mocha_draw_wallpaper(cairo_surface_t *surface, const char *filename,
                          int width, int height);

#endif  // CLIENT_H
