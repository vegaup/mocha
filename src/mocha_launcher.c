#include "mocha_launcher.h"

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include <X11/keysym.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "main.h"
#include "util/app.h"
#include "util/config.h"

void round_corners(cairo_t *cr, int x, int y, int width, int height,
                   double radius) {
    double degrees = M_PI / 180.0;
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + width - radius, y + radius, radius, -90 * degrees,
              0 * degrees);
    cairo_arc(cr, x + width - radius, y + height - radius, radius, 0 * degrees,
              90 * degrees);
    cairo_arc(cr, x + radius, y + height - radius, radius, 90 * degrees,
              180 * degrees);
    cairo_arc(cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
    cairo_close_path(cr);
}

void show_launcher(Display *dpy, int screen) {
    find_applications();

    Window root = RootWindow(dpy, screen);
    int width = 500;
    int height = 400;
    int taskbar_height = 60;
    int padding = 10;
    int x = padding;
    int y = DisplayHeight(dpy, screen) - height - taskbar_height - padding;

    XSetWindowAttributes attrs;
    attrs.override_redirect = True;
    attrs.background_pixel = 0;
    attrs.border_pixel = 0;
    attrs.colormap = argb_colormap;

    Window win = XCreateWindow(
        dpy, root, x, y, width, height, 0, argb_depth, InputOutput, argb_visual,
        CWOverrideRedirect | CWBackPixel | CWBorderPixel | CWColormap, &attrs);

    XSelectInput(
        dpy, win,
        ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask);
    XMapWindow(dpy, win);

    cairo_surface_t *surface =
        cairo_xlib_surface_create(dpy, win, argb_visual, width, height);
    cairo_t *cr = cairo_create(surface);

    int scroll_y = 0;
    bool running = true;
    while(running) {
        XEvent e;
        XNextEvent(dpy, &e);

        if(e.type == Expose) {
            cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
            cairo_set_source_rgba(cr, 0, 0, 0, 0);
            cairo_paint(cr);

            round_corners(cr, 0, 0, width, height, 15);
            cairo_set_source_rgba(cr, 0.1, 0.1, 0.1, 0.9);
            cairo_fill(cr);
            cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

            int columns = 5;
            int icon_size = 64;
            int h_padding = 20;
            int v_padding = 20;
            int text_height = 30;
            int item_width = (width - (columns + 1) * h_padding) / columns;
            int item_height = icon_size + text_height;

            for(int i = 0; i < app_count; i++) {
                int col = i % columns;
                int row = i / columns;
                int app_x = h_padding + col * (item_width + h_padding);
                int app_y =
                    v_padding + row * (item_height + v_padding) - scroll_y;

                if(app_y + item_height < 0 || app_y > height) continue;

                load_app_icon(&apps[i], icon_size);

                cairo_save(cr);
                cairo_arc(cr, app_x + icon_size / 2.0, app_y + icon_size / 2.0,
                          icon_size / 2.0, 0, 2 * M_PI);
                cairo_clip(cr);

                if(apps[i].icon_surface) {
                    cairo_set_source_surface(cr, apps[i].icon_surface, app_x,
                                             app_y);
                    cairo_paint(cr);
                } else {
                    cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
                    cairo_rectangle(cr, app_x, app_y, icon_size, icon_size);
                    cairo_fill(cr);
                }
                cairo_restore(cr);

                char truncated_name[256];
                strncpy(truncated_name, apps[i].name,
                        sizeof(truncated_name) - 1);
                truncated_name[sizeof(truncated_name) - 1] = '\0';

                cairo_text_extents_t extents;
                cairo_select_font_face(cr, "sans-serif",
                                       CAIRO_FONT_SLANT_NORMAL,
                                       CAIRO_FONT_WEIGHT_NORMAL);
                cairo_set_font_size(cr, 12);

                int max_text_width = item_width + 10;
                for(int len = strlen(truncated_name); len > 0; len--) {
                    cairo_text_extents(cr, truncated_name, &extents);
                    if(extents.width <= max_text_width) {
                        break;
                    }
                    truncated_name[len - 1] = '\0';
                }

                if(strlen(truncated_name) < strlen(apps[i].name) &&
                   strlen(truncated_name) > 3) {
                    truncated_name[strlen(truncated_name) - 3] = '\0';
                    strcat(truncated_name, "...");
                }

                cairo_text_extents(cr, truncated_name, &extents);
                cairo_set_source_rgb(cr, 1, 1, 1);
                double text_x = app_x + (icon_size - extents.width) / 2;
                cairo_move_to(cr, text_x, app_y + icon_size + 15);
                cairo_show_text(cr, truncated_name);
            }
            cairo_surface_flush(surface);

        } else if(e.type == KeyPress) {
            if(XLookupKeysym(&e.xkey, 0) == XK_Escape) {
                running = false;
            }
        } else if(e.type == ButtonPress) {
            if(e.xbutton.button == 4) scroll_y -= 30;
        } else if(e.xbutton.button == 5) {
            scroll_y += 30;
        }

        int columns = 5;
        int icon_size = 64;
        int v_padding = 20;
        int text_height = 30;
        int item_height = icon_size + text_height;
        int total_rows = (app_count + columns - 1) / columns;
        int max_scroll =
            total_rows * (item_height + v_padding) - height + v_padding;
        if(scroll_y < 0) scroll_y = 0;
        if(max_scroll < 0) max_scroll = 0;
        if(scroll_y > max_scroll) scroll_y = max_scroll;

        if(e.xbutton.button == 1) {
            int h_padding = 20;
            int item_width = (width - (columns + 1) * h_padding) / columns;

            for(int i = 0; i < app_count; i++) {
                int col = i % columns;
                int row = i / columns;
                int app_x = h_padding + col * (item_width + h_padding);
                int app_y =
                    v_padding + row * (item_height + v_padding) - scroll_y;

                if(e.xbutton.x >= app_x && e.xbutton.x <= app_x + icon_size &&
                   e.xbutton.y >= app_y && e.xbutton.y <= app_y + icon_size) {
                    pid_t pid = fork();
                    if(pid == 0) {
                        setsid();
                        execl("/bin/sh", "sh", "-c", apps[i].exec,
                              (char *)NULL);
                        perror("execl");
                        exit(1);
                    } else if(pid > 0) {
                        running = false;
                    }
                    break;
                }
            }
        }
        XEvent ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = Expose;
        ev.xexpose.window = win;
        XSendEvent(dpy, win, False, ExposureMask, &ev);
        XFlush(dpy);
    }
}