#include "util/client.h"

#include <X11/Xlib.h>

ClientState client_states[MAX_CLIENTS];
Window managed_clients[MAX_CLIENTS];
int num_managed_clients = 0;

ClientState *mocha_get_client_state(Window w) {
    return &client_states[w % MAX_CLIENTS];
}

/**
 * Add a managed client
 */
void mocha_add_managed_client(Window w) {
    if(num_managed_clients < MAX_CLIENTS) {
        managed_clients[num_managed_clients++] = w;
    }
}

/**
 * Remove a managed client
 */
void mocha_remove_managed_client(Window w) {
    for(int i = 0; i < num_managed_clients; i++) {
        if(managed_clients[i] == w) {
            // shift the rest down
            for(int j = i; j < num_managed_clients - 1; j++) {
                managed_clients[j] = managed_clients[j + 1];
            }
            num_managed_clients--;
            break;
        }
    }
}

/**
 * Get a clients title/name
 */
char *mocha_get_client_name(Window w) {
    char *name = NULL;
    if(XFetchName(dpy, w, &name) > 0 && name != NULL) {
        return name;
    }
    return "(Mocha Window)";
}

/**
 * A util to tile windows/clients
 */
void mocha_tile_clients(int taskbar_height) {
    ClientState *c;
    Window w;
    int count = 0;
    int screen = DefaultScreen(dpy);

    mocha_for_each_client(c, w) { count++; }
    mocha_for_each_client_end

    if(count == 0) return;

    int screen_w = DisplayWidth(dpy, screen);
    int screen_h = DisplayHeight(dpy, screen) - taskbar_height;

    // padding: 10px gap, 95% of screen, centered (it looks floaty :D)
    int gap = 10;
    int usable_w = (int)(screen_w * 0.95) - 2 * gap;
    int usable_h = (int)(screen_h * 0.95) - 2 * gap;
    int offset_x = (screen_w - usable_w) / 2;
    int offset_y = taskbar_height + (screen_h - usable_h) / 2;

    int master_w = count > 1 ? usable_w / 2 - gap / 2 : usable_w;
    int stack_w = usable_w - master_w - gap;
    int stack_count = count - 1;
    int stack_h = stack_count > 0 ? (usable_h - (stack_count - 1) * gap) / stack_count : 0;

    int i = 0;
    mocha_for_each_client(c, w) {
        if(i == 0) {
            XMoveResizeWindow(dpy, w,
                offset_x,
                offset_y,
                master_w - 2 * BORDER_WIDTH,
                usable_h - 2 * BORDER_WIDTH);
        } else {
            XMoveResizeWindow(dpy, w,
                offset_x + master_w + gap,
                offset_y + (i - 1) * (stack_h + gap),
                stack_w - 2 * BORDER_WIDTH,
                stack_h - 2 * BORDER_WIDTH);
        }
        i++;
    }
    mocha_for_each_client_end
}
