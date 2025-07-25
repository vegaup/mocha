#ifndef EVENT_H
#define EVENT_H

#include <X11/Xlib.h>

struct DragState {
    Window active_window;
    int dragging, resizing;
    int start_x, start_y;
    int win_x, win_y, win_w, win_h;
};

void mocha_handle_event(XEvent event, Window taskbar,
                        struct DragState *drag_state, int taskbar_height,
                        int tiling_enabled);

#endif  // EVENT_H