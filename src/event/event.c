#include "event/event.h"

#include <X11/Xlib.h>

#include "event/event.h"
#include "features/launcher.h"
#include "main.h"
#include "ui/toast.h"

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

void mocha_handle_event(XEvent event, Window taskbar, Window active_window,
                        int taskbar_height, int start_x, int start_y, int win_x,
                        int win_y, int win_w, int win_h, int dragging,
                        int resizing, int tiling_enabled) {
    switch(event.type) {
        case ButtonPress: {
            XButtonEvent *e = &event.xbutton;
            Window child;
            int root_x, root_y, win_rel_x, win_rel_y;
            unsigned int mask;

            if(XQueryPointer(dpy, root, &root, &child, &root_x, &root_y,
                             &win_rel_x, &win_rel_y, &mask)) {
                if(child != None) {
                    XWindowAttributes attr;
                    XGetWindowAttributes(dpy, child, &attr);
                    active_window = child;
                    start_x = e->x_root;
                    start_y = e->y_root;
                    win_x = attr.x;
                    win_y = attr.y;
                    win_w = attr.width;
                    win_h = attr.height;

                    if(tiling_enabled) {
                        if((e->state & Mod1Mask) && e->button == Button1) {
                            dragging = True;
                        } else if(e->button == Button3) {
                            resizing = True;
                        }
                    } else {
                        if(e->button == Button1) {
                            dragging = True;
                        } else if(e->button == Button3) {
                            resizing = True;
                        }
                    }
                }
            }

            if(event.xbutton.window == taskbar) {
                int click_x = event.xbutton.x;

                int x = 5;
                ClientState *c;
                Window w;
                mocha_for_each_client(c, w) {
                    char *name = mocha_get_client_name(w);
                    int width = strlen(name) * 8 + 10;
                    if(click_x >= x && click_x < x + width) {
                        XSetInputFocus(dpy, w, RevertToPointerRoot,
                                       CurrentTime);
                        XRaiseWindow(dpy, w);
                        break;
                    }
                    x += width;
                }
                mocha_for_each_client_end
            }

            if(event.xbutton.window == taskbar) {
                int click_x = event.xbutton.x;
                int click_y = event.xbutton.y;

                if(click_x >= 5 && click_x <= 85 && click_y >= 3 &&
                   click_y <= 27) {
                    mocha_launch_menu();
                    break;
                }

                int x = 5 + 80 + 10;
                ClientState *c;
                Window w;
                mocha_for_each_client(c, w) {
                    char *name = mocha_get_client_name(w);
                    int width = strlen(name) * 8 + 10;
                    if(click_x >= x && click_x < x + width) {
                        XSetInputFocus(dpy, w, RevertToPointerRoot,
                                       CurrentTime);
                        XRaiseWindow(dpy, w);
                        break;
                    }
                    x += width;
                }
                mocha_for_each_client_end
            }

            XAllowEvents(dpy, ReplayPointer, CurrentTime);
            break;
        }

        case ButtonRelease:
            dragging = resizing = 0;
            break;

        case MotionNotify: {
            if(active_window != None) {
                int dx = event.xmotion.x_root - start_x;
                int dy = event.xmotion.y_root - start_y;

                if(dragging) {
                    XMoveWindow(dpy, active_window, win_x + dx, win_y + dy);
                } else if(resizing) {
                    XResizeWindow(dpy, active_window,
                                  win_w + dx > 50 ? win_w + dx : 50,
                                  win_h + dy > 50 ? win_h + dy : 50);
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
            if(XQueryPointer(dpy, root, &root, &mouse_win, &rx, &ry, &wx, &wy, &mask)) {
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
                ClientState *state = mocha_get_client_state(mouse_win);
                int border = BORDER_WIDTH;
                if(state->is_fullscreen) {
                    XMoveResizeWindow(dpy, mouse_win, state->saved_x,
                                      state->saved_y, state->saved_w,
                                      state->saved_h);
                    state->is_fullscreen = False;
                } else {
                    XWindowAttributes attr;
                    XGetWindowAttributes(dpy, mouse_win, &attr);
                    state->saved_x = attr.x;
                    state->saved_y = attr.y;
                    state->saved_w = attr.width;
                    state->saved_h = attr.height;
                    int screen_w = DisplayWidth(dpy, screen);
                    int screen_h = DisplayHeight(dpy, screen);
                    XMoveResizeWindow(dpy, mouse_win, 0, 0, screen_w - 2 * border,
                                      screen_h - 2 * border);
                    state->is_fullscreen = True;
                }
                update_window_borders(mouse_win);
            } else if((event.xkey.state & Mod1Mask) && keysym == XK_x &&
                      mouse_win != None && mouse_win != PointerRoot) {
                XDestroyWindow(dpy, mouse_win);
            }
            break;
        }

        case MapRequest: {
            XMapRequestEvent *e = &event.xmaprequest;
            mocha_add_managed_client(e->window);
            XSetWindowBorderWidth(dpy, e->window, BORDER_WIDTH);
            XSetWindowBorder(dpy, e->window, border_color);
            if(tiling_enabled) mocha_tile_clients(taskbar_height);
            XMapWindow(dpy, e->window);
            XSetInputFocus(dpy, e->window, RevertToPointerRoot, CurrentTime);
            XSetWindowBorder(dpy, e->window, focus_color);
            break;
        }

        case ConfigureRequest: {
            XConfigureRequestEvent *e = &event.xconfigurerequest;
            XWindowChanges changes;
            changes.x = e->x;
            changes.y = e->y;
            changes.width = e->width;
            changes.height = e->height;
            changes.border_width = BORDER_WIDTH;
            changes.sibling = e->above;
            changes.stack_mode = e->detail;
            XConfigureWindow(dpy, e->window, e->value_mask | CWBorderWidth,
                             &changes);
            break;
        }

        case EnterNotify: {
            XCrossingEvent *e = &event.xcrossing;
            XSetInputFocus(dpy, e->window, RevertToPointerRoot, CurrentTime);
            update_window_borders(e->window);
            break;
        }

        case Expose: {
            if(event.xexpose.window == taskbar) {
                GC gc = DefaultGC(dpy, screen);

                XSetForeground(dpy, gc, panel_color);
                XFillRectangle(dpy, taskbar, gc, 0, 0,
                               DisplayWidth(dpy, screen), taskbar_height);

                int btn_x = 5, btn_y = 3, btn_w = 80, btn_h = 24;
                XSetForeground(dpy, gc, foreground_color);
                XFillRectangle(dpy, taskbar, gc, btn_x, btn_y, btn_w, btn_h);

                char *btn_label = "Launcher";
                XSetForeground(dpy, gc, panel_color);
                XDrawString(dpy, taskbar, gc, btn_x + 10, btn_y + 16, btn_label,
                            strlen(btn_label));

                int x = btn_x + btn_w + 10;
                ClientState *c;
                Window w;
                mocha_for_each_client(c, w) {
                    char *name = mocha_get_client_name(w);
                    if(name) {
                        XSetForeground(dpy, gc, focus_color);
                        XDrawString(dpy, taskbar, gc, x, 20, name,
                                    strlen(name));
                        x += strlen(name) * 8 + 10;
                    }
                }
                mocha_for_each_client_end
            } else {
                Toast *t = toasts;
                XExposeEvent *e = &event.xexpose;

                while(t) {
                    if(t->win == e->window) {
                        GC gc = XCreateGC(dpy, t->win, 0, NULL);
                        XSetForeground(dpy, gc, foreground_color);
                        XDrawString(dpy, t->win, gc, 10, 30, t->message,
                                    strlen(t->message));
                        XFreeGC(dpy, gc);
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
            break;
        }

        default:
            break;
    }

    XSync(dpy, 0);
}
