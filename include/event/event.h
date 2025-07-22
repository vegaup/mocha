#ifndef EVENT_H
#define EVENT_H

#include <X11/Xlib.h>

extern int dragging, resizing;
extern int start_x, start_y;
extern int win_x, win_y, win_w, win_h;

void mocha_handle_event(XEvent event, Window taskbar, Window active_window,
                        int taskbar_height, int start_x, int start_y, int win_x,
                        int win_y, int win_w, int win_h, int dragging,
                        int resizing, int tiling_enabled);

#endif  // EVENT_H