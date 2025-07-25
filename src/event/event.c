#include "event/event.h"

#include <X11/XF86keysym.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include <X11/keysym.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "features/launcher.h"
#include "main.h"
#include "ui/toast.h"
#include "util/client.h"
#include "util/config.h"

extern int screen;
extern Display *dpy;
extern unsigned long border_color, focus_color, panel_color, foreground_color,
    accent_color;

extern Window quote_win;

void update_window_borders(Window focused);
static void round_corners(Window win, int width, int height, int radius);

static void minimize_window(Window w) {
    ClientState *state = mocha_get_client_state(w);
    if(!state->is_minimized) {
        XUnmapWindow(dpy, w);
        state->is_minimized = 1;
    }
}

static void restore_window(Window w) {
    ClientState *state = mocha_get_client_state(w);
    if(state->is_minimized) {
        XMapWindow(dpy, w);
        XRaiseWindow(dpy, w);
        XSetInputFocus(dpy, w, RevertToPointerRoot, CurrentTime);
        state->is_minimized = 0;
    }
}

static void toggle_maximize_window(Window w) {
    ClientState *state = mocha_get_client_state(w);
    int border = get_border_width();
    if(state->is_fullscreen) {
        XMoveResizeWindow(dpy, w, state->saved_x, state->saved_y,
                          state->saved_w, state->saved_h);
        state->is_fullscreen = 0;
        round_corners(w, state->saved_w, state->saved_h,
                      config.features.border_radius);
    } else {
        XWindowAttributes attr;
        XGetWindowAttributes(dpy, w, &attr);
        state->saved_x = attr.x;
        state->saved_y = attr.y;
        state->saved_w = attr.width;
        state->saved_h = attr.height;
        int screen_w = DisplayWidth(dpy, screen);
        int screen_h = DisplayHeight(dpy, screen);
        XMoveResizeWindow(dpy, w, 0, 0, screen_w - 2 * border,
                          screen_h - 2 * border);
        state->is_fullscreen = 1;
        round_corners(w, screen_w - 2 * border, screen_h - 2 * border, 0);
    }
    update_window_borders(w);
}

static int is_managed_client(Window w) {
    for(int i = 0; i < num_managed_clients; ++i) {
        if(managed_clients[i] == w) return 1;
    }
    return 0;
}

void update_window_borders(Window focused) {
    ClientState *c;
    Window w;
    mocha_for_each_client(c, w) {
        if(w == focused) {
            XSetWindowBorder(dpy, w, focus_color);
        } else {
            XSetWindowBorder(dpy, w, border_color);
        }
    }
    mocha_for_each_client_end
}

static void round_corners(Window win, int width, int height, int radius) {
    if(radius <= 0) {
        XShapeCombineMask(dpy, win, ShapeBounding, 0, 0, None, ShapeSet);
        return;
    }

    Pixmap shape_mask = XCreatePixmap(dpy, win, width, height, 1);
    if(shape_mask == None) {
        return;
    }

    GC gc = XCreateGC(dpy, shape_mask, 0, NULL);
    if(gc == NULL) {
        XFreePixmap(dpy, shape_mask);
        return;
    }

    XSetForeground(dpy, gc, 0);
    XFillRectangle(dpy, shape_mask, gc, 0, 0, width, height);

    XSetForeground(dpy, gc, 1);
    XFillArc(dpy, shape_mask, gc, 0, 0, 2 * radius, 2 * radius, 90 * 64,
             90 * 64);
    XFillArc(dpy, shape_mask, gc, width - 2 * radius - 1, 0, 2 * radius,
             2 * radius, 0 * 64, 90 * 64);
    XFillArc(dpy, shape_mask, gc, 0, height - 2 * radius - 1, 2 * radius,
             2 * radius, 180 * 64, 90 * 64);
    XFillArc(dpy, shape_mask, gc, width - 2 * radius - 1,
             height - 2 * radius - 1, 2 * radius, 2 * radius, 270 * 64,
             90 * 64);

    XFillRectangle(dpy, shape_mask, gc, radius, 0, width - 2 * radius, height);
    XFillRectangle(dpy, shape_mask, gc, 0, radius, width, height - 2 * radius);

    XShapeCombineMask(dpy, win, ShapeBounding, 0, 0, shape_mask, ShapeSet);

    XFreeGC(dpy, gc);
    XFreePixmap(dpy, shape_mask);
}

static void show_volume_toast() {
    char buf[128];
    if(run_command("amixer get Master | grep -o '[0-9]*%' | head -1", buf,
                   sizeof(buf)) == 0 &&
       strlen(buf) > 0) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Volume: %s", buf);
        size_t len = strlen(msg);
        if(len > 0 && msg[len - 1] == '\n') msg[len - 1] = '\0';
        show_toast(msg);
        return;
    }
    if(run_command(
           "pactl get-sink-volume @DEFAULT_SINK@ | grep -o '[0-9]*%' | head -1",
           buf, sizeof(buf)) == 0 &&
       strlen(buf) > 0) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Volume: %s", buf);
        size_t len = strlen(msg);
        if(len > 0 && msg[len - 1] == '\n') msg[len - 1] = '\0';
        show_toast(msg);
        return;
    }
    show_toast("Volume: ?");
}

void mocha_handle_event(XEvent event, Window taskbar,
                        struct DragState *drag_state, int taskbar_height,
                        int tiling_enabled) {
    switch(event.type) {
        case ButtonPress: {
            XButtonEvent *e = &event.xbutton;
            Window child;
            int root_x, root_y, win_rel_x, win_rel_y;
            unsigned int mask;

            if(XQueryPointer(dpy, root, &root, &child, &root_x, &root_y,
                             &win_rel_x, &win_rel_y, &mask)) {
                if(child != None && is_managed_client(child)) {
                    XWindowAttributes attr;
                    XGetWindowAttributes(dpy, child, &attr);
                    drag_state->active_window = child;
                    drag_state->start_x = e->x_root;
                    drag_state->start_y = e->y_root;
                    drag_state->win_x = attr.x;
                    drag_state->win_y = attr.y;
                    drag_state->win_w = attr.width;
                    drag_state->win_h = attr.height;

                    if(tiling_enabled) {
                        if((e->state & Mod1Mask) && e->button == Button1) {
                            drag_state->dragging = True;
                        } else if(e->button == Button3) {
                            drag_state->resizing = True;
                        }
                    } else {
                        if(e->button == Button1) {
                            drag_state->dragging = True;
                        } else if(e->button == Button3) {
                            drag_state->resizing = True;
                        }
                    }
                } else {
                    drag_state->active_window = None;
                }
            }

            if(event.xbutton.window == taskbar) {
                mocha_handle_dock_click(event.xbutton.x, event.xbutton.y);
            }

            XAllowEvents(dpy, ReplayPointer, CurrentTime);
            break;
        }

        case ButtonRelease:
            drag_state->dragging = 0;
            drag_state->resizing = 0;
            break;

        case MotionNotify: {
            if(drag_state->active_window != None &&
               is_managed_client(drag_state->active_window)) {
                int dx = event.xmotion.x_root - drag_state->start_x;
                int dy = event.xmotion.y_root - drag_state->start_y;

                if(drag_state->dragging) {
                    XMoveWindow(dpy, drag_state->active_window,
                                drag_state->win_x + dx, drag_state->win_y + dy);
                } else if(drag_state->resizing) {
                    XResizeWindow(
                        dpy, drag_state->active_window,
                        drag_state->win_w + dx > 50 ? drag_state->win_w + dx
                                                    : 50,
                        drag_state->win_h + dy > 50 ? drag_state->win_h + dy
                                                    : 50);
                    int new_width = drag_state->win_w + dx > 50
                                        ? drag_state->win_w + dx
                                        : 50;
                    int new_height = drag_state->win_h + dy > 50
                                         ? drag_state->win_h + dy
                                         : 50;
                    round_corners(drag_state->active_window, new_width,
                                  new_height, config.features.border_radius);
                }
            }
            break;
        }

        case KeyPress: {
            KeySym keysym = XLookupKeysym(&event.xkey, 0);
            Window focused;
            int revert;
            XGetInputFocus(dpy, &focused, &revert);
            Window mouse_win = None;
            int rx, ry, wx, wy;
            unsigned int mask;
            if(XQueryPointer(dpy, root, &root, &mouse_win, &rx, &ry, &wx, &wy,
                             &mask)) {
                if(mouse_win == None || mouse_win == root) mouse_win = focused;
            } else {
                mouse_win = focused;
            }
            if((event.xkey.state & Mod1Mask) && keysym == XK_0) {
                mocha_shutdown();
            } else if((event.xkey.state & Mod1Mask) && keysym == XK_q) {
                system("ghostty &");
            } else if((event.xkey.state & Mod1Mask) && keysym == XK_c) {
                system("chromium &");
            } else if((event.xkey.state & Mod1Mask) && keysym == XK_z) {
                mocha_launch_menu();
            } else if((event.xkey.state & Mod1Mask) && keysym == XK_f &&
                      mouse_win != None && mouse_win != PointerRoot) {
                toggle_maximize_window(mouse_win);
            } else if((event.xkey.state & Mod1Mask) && keysym == XK_x &&
                      mouse_win != None && mouse_win != PointerRoot) {
                XDestroyWindow(dpy, mouse_win);
            } else if((event.xkey.state & Mod1Mask) && keysym == XK_m &&
                      mouse_win != None && mouse_win != PointerRoot) {
                minimize_window(mouse_win);
            } else if(keysym == XF86XK_AudioRaiseVolume) {
                system(
                    "amixer set Master 5%+ || pactl set-sink-volume "
                    "@DEFAULT_SINK@ +5% > /dev/null");
                show_volume_toast();
            } else if(keysym == XF86XK_AudioLowerVolume) {
                system(
                    "amixer set Master 5%- || pactl set-sink-volume "
                    "@DEFAULT_SINK@ -5% > /dev/null");
                show_volume_toast();
            } else if(keysym == XF86XK_AudioMute) {
                system(
                    "amixer set Master toggle || pactl set-sink-mute "
                    "@DEFAULT_SINK@ toggle > /dev/null");
                show_volume_toast();
            }
            break;
        }

        case MapRequest: {
            XMapRequestEvent *e = &event.xmaprequest;

            if(is_dialog(e->window)) {
                system("thunar &");
                XDestroyWindow(dpy, e->window);
                break;
            }

            mocha_add_managed_client(e->window);
            XSetWindowBorderWidth(dpy, e->window, get_border_width());
            XSetWindowBorder(dpy, e->window, border_color);
            if(tiling_enabled) mocha_tile_clients(taskbar_height);
            XMapWindow(dpy, e->window);
            XWindowAttributes attr;
            XGetWindowAttributes(dpy, e->window, &attr);
            round_corners(e->window, attr.width, attr.height,
                          config.features.border_radius);
            XSetInputFocus(dpy, e->window, RevertToPointerRoot, CurrentTime);
            XSetWindowBorder(dpy, e->window, focus_color);
            mocha_update_dock_icons();
            break;
        }

        case ConfigureRequest: {
            XConfigureRequestEvent *e = &event.xconfigurerequest;
            XWindowChanges changes;
            changes.x = e->x;
            changes.y = e->y;
            changes.width = e->width;
            changes.height = e->height;
            changes.border_width = get_border_width();
            changes.sibling = e->above;
            changes.stack_mode = e->detail;
            XConfigureWindow(dpy, e->window, e->value_mask | CWBorderWidth,
                             &changes);
            round_corners(e->window, e->width, e->height,
                          config.features.border_radius);
            break;
        }

        case EnterNotify: {
            XCrossingEvent *e = &event.xcrossing;
            XSetInputFocus(dpy, e->window, RevertToPointerRoot, CurrentTime);
            update_window_borders(e->window);
            break;
        }

        case Expose: {
            if(event.xexpose.window == root) {
                XClearWindow(dpy, root);
                if(quote_win) {
                    XEvent expose_event;
                    memset(&expose_event, 0, sizeof(expose_event));
                    expose_event.type = Expose;
                    expose_event.xexpose.window = quote_win;
                    XSendEvent(dpy, quote_win, False, ExposureMask,
                               &expose_event);
                    XFlush(dpy);
                }
                break;
            } else if(event.xexpose.window == taskbar) {
                mocha_draw_dock(taskbar);
            } else if(quote_win && event.xexpose.window == quote_win) {
                draw_quote_window();
            } else {
                Toast *t = toasts;
                XExposeEvent *e = &event.xexpose;

                while(t) {
                    if(t->win == e->window) {
                        XftDraw *draw = XftDrawCreate(
                            dpy, t->win, DefaultVisual(dpy, screen),
                            DefaultColormap(dpy, screen));
                        if(!draw) {
                            fprintf(stderr, "XftDrawCreate (toast) failed\n");
                            return;
                        }
                        int font_size = 16;
                        char font_pattern[128];
                        snprintf(font_pattern, sizeof(font_pattern),
                                 "DejaVuSansMono:size=%d", font_size);
                        XftFont *font =
                            XftFontOpenName(dpy, screen, font_pattern);
                        if(!font) {
                            fprintf(stderr,
                                    "XftFontOpenName (toast) failed for '%s', "
                                    "trying 'sans'\n",
                                    font_pattern);
                            snprintf(font_pattern, sizeof(font_pattern),
                                     "sans:size=%d", font_size);
                            font = XftFontOpenName(dpy, screen, font_pattern);
                        }
                        if(!font) {
                            fprintf(stderr,
                                    "XftFontOpen (toast) failed for both "
                                    "'DejaVu Sans Mono' and 'sans'\n");
                            XftDrawDestroy(draw);
                            return;
                        }
                        XftColor xft_fg;
                        XRenderColor render_fg = {.red = 0xFFFF,
                                                  .green = 0xFFFF,
                                                  .blue = 0xFFFF,
                                                  .alpha = 0xFFFF};
                        if(!XftColorAllocValue(dpy, DefaultVisual(dpy, screen),
                                               DefaultColormap(dpy, screen),
                                               &render_fg, &xft_fg)) {
                            fprintf(stderr,
                                    "XftColorAllocValue (toast fg) failed, "
                                    "using white\n");
                            xft_fg.color.red = 0xFFFF;
                            xft_fg.color.green = 0xFFFF;
                            xft_fg.color.blue = 0xFFFF;
                            xft_fg.color.alpha = 0xFFFF;
                        }
                        int y_offset = 30;
                        XftDrawStringUtf8(draw, &xft_fg, font, 10, y_offset,
                                          (FcChar8 *)t->message,
                                          strlen(t->message));
                        XftFontClose(dpy, font);
                        XftDrawDestroy(draw);
                        break;
                    }
                    t = t->next;
                }
            }
            break;
        }

        case LeaveNotify: {
            XCrossingEvent *e = &event.xcrossing;
            update_window_borders(None);
            break;
        }

        case DestroyNotify: {
            XDestroyWindowEvent *e = &event.xdestroywindow;
            mocha_remove_managed_client(e->window);
            if(tiling_enabled) mocha_tile_clients(taskbar_height);
            XEvent expose_event;
            expose_event.type = Expose;
            expose_event.xexpose.window = taskbar;
            XSendEvent(dpy, taskbar, False, ExposureMask, &expose_event);
            XFlush(dpy);
            mocha_update_dock_icons();
            break;
        }

        default:
            break;
    }

    XSync(dpy, 0);
}
