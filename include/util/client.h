#ifndef CLIENT_H
#define CLIENT_H

#include "main.h"

/* Maximum amount of clients to allow at once */
#define MAX_CLIENTS 1024
/* Macro to iterate through all clients */
#define mocha_for_each_client(cvar, wvar)                \
    for(int __i = 0; __i < num_managed_clients; __i++) { \
        Window wvar = managed_clients[__i];              \
        ClientState *cvar = mocha_get_client_state(wvar);
#define mocha_for_each_client_end }

/* Struct to keep track of client state */
typedef struct {
    int is_fullscreen;
    int saved_x, saved_y, saved_w, saved_h;
} ClientState;

/* All client states */
extern ClientState client_states[MAX_CLIENTS];
/* 1 client state */
extern ClientState *mocha_get_client_state(Window w);
/* All windows being managed */
extern Window managed_clients[MAX_CLIENTS];
/* Number of managed clients */
extern int num_managed_clients;

void mocha_add_managed_client(Window w);
void mocha_remove_managed_client(Window w);
void mocha_tile_clients(int taskbar_height);
char *mocha_get_client_name(Window w);

#endif  // CLIENT_H
