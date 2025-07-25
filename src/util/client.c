#include "util/client.h"

#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "features/launcher.h"
#include "util/app.h"
#include "util/config.h"
#define STB_IMAGE_IMPLEMENTATION
#include "lib/stb_image.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern int screen;
extern Display *dpy;
extern unsigned long border_color, focus_color, panel_color, foreground_color,
    accent_color;

ClientState client_states[MAX_CLIENTS] = {0};
Window managed_clients[MAX_CLIENTS];
int num_managed_clients = 0;

DockIcon dock_icons[MAX_DOCK_ICONS] = {0};
int num_dock_icons = 0;

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

static char *get_wm_class(Window win) {
    XClassHint class_hint;
    if(XGetClassHint(dpy, win, &class_hint)) {
        char *wm_class = strdup(class_hint.res_class);
        XFree(class_hint.res_name);
        XFree(class_hint.res_class);
        return wm_class;
    }
    return NULL;
}

int is_dialog(Window w) {
    Atom type;
    int format;
    unsigned long nitems, bytes_after;
    unsigned char *prop;
    Atom dialog_atom = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);

    if(XGetWindowProperty(dpy, w,
                          XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False), 0,
                          sizeof(Atom), False, XA_ATOM, &type, &format, &nitems,
                          &bytes_after, &prop) == Success &&
       prop) {
        Atom window_type = *(Atom *)prop;
        XFree(prop);
        if(window_type == dialog_atom) {
            return 1;
        }
    }

    return 0;
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

    int gap = 10;
    int usable_w = (int)(screen_w * 0.95) - 2 * gap;
    int usable_h = (int)(screen_h * 0.95) - 2 * gap;
    int offset_x = (screen_w - usable_w) / 2;
    int offset_y = (screen_h - usable_h) / 2;

    int master_w = count > 1 ? usable_w / 2 - gap / 2 : usable_w;
    int stack_w = usable_w - master_w - gap;
    int stack_count = count - 1;
    int stack_h = stack_count > 0
                      ? (usable_h - (stack_count - 1) * gap) / stack_count
                      : 0;

    int i = 0;
    mocha_for_each_client(c, w) {
        if(i == 0) {
            XMoveResizeWindow(dpy, w, offset_x, offset_y,
                              master_w - 2 * get_border_width(),
                              usable_h - 2 * get_border_width());
        } else {
            XMoveResizeWindow(dpy, w, offset_x + master_w + gap,
                              offset_y + (i - 1) * (stack_h + gap),
                              stack_w - 2 * get_border_width(),
                              stack_h - 2 * get_border_width());
        }
        i++;
    }
    mocha_for_each_client_end
}

/**
 * Draw a circular icon using Cairo
 */
static void draw_circular_icon(cairo_t *cr, int x, int y, int size,
                               unsigned int color, const char *label,
                               bool is_launcher) {
    if(cr == NULL || cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
        return;
    }

    cairo_save(cr);

    double r = ((accent_color >> 16) & 0xFF) / 255.0;
    double g = ((accent_color >> 8) & 0xFF) / 255.0;
    double b = (accent_color & 0xFF) / 255.0;
    double a = 1.0;

    if(is_launcher) {
        cairo_set_source_rgba(cr, r, g, b, a);
        cairo_arc(cr, x + size / 2, y + size / 2, size / 2, 0, 2 * M_PI);
        cairo_fill(cr);

        cairo_set_source_rgba(cr, 1, 1, 1, 0.9);
        cairo_arc(cr, x + size / 2, y + size / 2, size / 3, 0, 2 * M_PI);
        cairo_set_line_width(cr, 2);
        cairo_stroke(cr);
    } else {
        cairo_set_source_rgba(cr, r, g, b, a);
        cairo_arc(cr, x + size / 2, y + size / 2, size / 2, 0, 2 * M_PI);
        cairo_fill(cr);

        if(label && label[0]) {
            cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                                   CAIRO_FONT_WEIGHT_BOLD);
            cairo_set_font_size(cr, size * 0.4);
            cairo_set_source_rgba(cr, 1, 1, 1, 0.9);

            cairo_text_extents_t extents;
            char icon_letter[2] = {label[0], '\0'};

            cairo_text_extents(cr, icon_letter, &extents);
            if(cairo_status(cr) == CAIRO_STATUS_SUCCESS) {
                double text_x =
                    x + (size - extents.width) / 2 - extents.x_bearing;
                double text_y =
                    y + (size - extents.height) / 2 - extents.y_bearing;

                cairo_move_to(cr, text_x, text_y);
                cairo_show_text(cr, icon_letter);
            }
        }
    }

    cairo_restore(cr);
}

/**
 * Initialize the dock with default applications
 */
void mocha_init_dock() {
    num_dock_icons = 0;

    mocha_add_dock_icon("Launcher", "");
    mocha_add_dock_icon("Terminal", "ghostty");
    mocha_add_dock_icon("Browser", "chromium");
    mocha_add_dock_icon("Files", "thunar");

    for(int i = 0; i < num_dock_icons; i++) {
        dock_icons[i].app = find_app_by_wmclass(dock_icons[i].name);
    }
}

/**
 * Add an icon to the dock
 */
void mocha_add_dock_icon(const char *name, const char *command) {
    if(num_dock_icons >= MAX_DOCK_ICONS) return;

    DockIcon *icon = &dock_icons[num_dock_icons++];
    strncpy(icon->name, name, sizeof(icon->name) - 1);
    strncpy(icon->command, command, sizeof(icon->command) - 1);
    icon->is_running = 0;
    icon->window = None;
}

/**
 * Draw the dock with app icons using Cairo
 */
void mocha_draw_dock(Window dock_win) {
    XWindowAttributes win_attrs;
    XGetWindowAttributes(dpy, dock_win, &win_attrs);
    int screen_w = DisplayWidth(dpy, screen);
    int screen_h = DisplayHeight(dpy, screen);
    int depth = win_attrs.depth;
    Visual *visual = win_attrs.visual;

    XSync(dpy, False);

    Pixmap pixmap = XCreatePixmap(dpy, dock_win, screen_w, 60, depth);
    if(pixmap == None) {
        mocha_log("Failed to create pixmap for dock rendering");
        return;
    }

    XErrorHandler old_handler = XSetErrorHandler(NULL);

    cairo_surface_t *surface =
        cairo_xlib_surface_create(dpy, pixmap, visual, screen_w, 60);

    XSetErrorHandler(old_handler);
    XSync(dpy, False);

    if(surface == NULL ||
       cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        mocha_log("Failed to create Cairo surface");
        XFreePixmap(dpy, pixmap);
        if(surface) cairo_surface_destroy(surface);
        return;
    }

    cairo_t *cr = cairo_create(surface);

    if(cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
        mocha_log("Failed to create Cairo context: %s",
                  cairo_status_to_string(cairo_status(cr)));
        cairo_destroy(cr);
        cairo_surface_destroy(surface);
        XFreePixmap(dpy, pixmap);
        return;
    }

    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    int icon_size = 28;
    int icon_spacing = 8;
    int dock_padding = 6;
    int dock_height = icon_size + dock_padding * 2;

    int dock_width = screen_w - 10;

    int dock_x = (screen_w - dock_width) / 2;
    int dock_y = 20;

    double corner_radius = dock_height / 2.0;

    double r = ((accent_color >> 16) & 0xFF) / 255.0;
    double g = ((accent_color >> 8) & 0xFF) / 255.0;
    double b = (accent_color & 0xFF) / 255.0;

    cairo_set_source_rgba(cr, r * 0.3, g * 0.3, b * 0.3, 0.7);
    cairo_set_line_width(cr, 0);
    cairo_move_to(cr, dock_x + corner_radius, dock_y + dock_height);
    cairo_arc(cr, dock_x + corner_radius, dock_y + corner_radius, corner_radius,
              M_PI, 1.5 * M_PI);
    cairo_arc(cr, dock_x + dock_width - corner_radius, dock_y + corner_radius,
              corner_radius, 1.5 * M_PI, 2 * M_PI);
    cairo_arc(cr, dock_x + dock_width - corner_radius,
              dock_y + dock_height - corner_radius, corner_radius, 0,
              0.5 * M_PI);
    cairo_arc(cr, dock_x + corner_radius, dock_y + dock_height - corner_radius,
              corner_radius, 0.5 * M_PI, M_PI);
    cairo_close_path(cr);
    cairo_fill(cr);

    int launcher_x = dock_x + dock_padding;
    int launcher_y = dock_y + dock_padding;

    draw_circular_icon(cr, launcher_x, launcher_y, icon_size, accent_color, "L",
                       true);
    dock_icons[0].x = launcher_x;
    dock_icons[0].y = launcher_y;

    int app_icons_width = (num_dock_icons - 1) * (icon_size + icon_spacing);
    int center_start_x = (screen_w - app_icons_width) / 2;

    for(int i = 1; i < num_dock_icons; i++) {
        int icon_x = center_start_x + (i - 1) * (icon_size + icon_spacing);
        int icon_y = dock_y + dock_padding;

        if(dock_icons[i].app) {
            load_app_icon(dock_icons[i].app, icon_size);
            if(dock_icons[i].app->icon_surface) {
                cairo_save(cr);
                cairo_arc(cr, icon_x + icon_size / 2.0,
                          icon_y + icon_size / 2.0, icon_size / 2.0, 0,
                          2 * M_PI);
                cairo_clip(cr);
                cairo_set_source_surface(cr, dock_icons[i].app->icon_surface,
                                         icon_x, icon_y);
                cairo_paint(cr);
                cairo_restore(cr);
            } else {
                draw_circular_icon(cr, icon_x, icon_y, icon_size, accent_color,
                                   dock_icons[i].name, false);
            }
        } else {
            draw_circular_icon(cr, icon_x, icon_y, icon_size, accent_color,
                               dock_icons[i].name, false);
        }

        dock_icons[i].x = icon_x;
        dock_icons[i].y = icon_y;
    }

    cairo_surface_flush(surface);

    GC gc = XCreateGC(dpy, dock_win, 0, NULL);
    XCopyArea(dpy, pixmap, dock_win, gc, 0, 0, screen_w, 60, 0, 0);
    XFreeGC(dpy, gc);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    XFreePixmap(dpy, pixmap);

    XSync(dpy, False);
}

/**
 * Handle clicks on the dock
 */
void mocha_handle_dock_click(int x, int y) {
    for(int i = 0; i < num_dock_icons; i++) {
        int icon_size = 28;
        int icon_center_x = dock_icons[i].x + icon_size / 2;
        int icon_center_y = dock_icons[i].y + icon_size / 2;

        double distance =
            sqrt(pow(x - icon_center_x, 2) + pow(y - icon_center_y, 2));
        if(distance <= icon_size / 2) {
            if(strcmp(dock_icons[i].name, "Launcher") == 0) {
                mocha_launch_menu();
            } else {
                char cmd[300];
                snprintf(cmd, sizeof(cmd), "%s &", dock_icons[i].command);
                system(cmd);
            }
            return;
        }
    }
}

void mocha_update_dock_icons() {
    for(int i = 0; i < num_dock_icons; i++) {
        dock_icons[i].is_running = 0;
    }
    ClientState *c;
    Window w;
    mocha_for_each_client(c, w) {
        char *wm_class = get_wm_class(w);
        if(wm_class) {
            AppInfo *app = find_app_by_wmclass(wm_class);
            if(app) {
                for(int i = 0; i < num_dock_icons; i++) {
                    if(dock_icons[i].app == app) {
                        dock_icons[i].is_running = 1;
                        break;
                    }
                }
            }
            free(wm_class);
        }
    }
    mocha_for_each_client_end
}

static void rgba_to_cairo_argb32(uint8_t *data, int w, int h) {
    for(int i = 0; i < w * h; ++i) {
        uint8_t r = data[4 * i + 0];
        uint8_t g = data[4 * i + 1];
        uint8_t b = data[4 * i + 2];
        uint8_t a = data[4 * i + 3];
        r = (uint8_t)((r * a) / 255);
        g = (uint8_t)((g * a) / 255);
        b = (uint8_t)((b * a) / 255);
        uint32_t argb =
            ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
        ((uint32_t *)data)[i] = argb;
    }
}

void mocha_draw_wallpaper(cairo_surface_t *surface, const char *filename,
                          int width, int height) {
    int img_w, img_h, img_channels;
    mocha_log("Loading wallpaper: %s", filename);
    stbi_uc *data = stbi_load(filename, &img_w, &img_h, &img_channels, 4);
    if(!data) {
        mocha_log("Failed to load wallpaper image: %s", filename);
        cairo_t *cr = cairo_create(surface);
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_paint(cr);
        cairo_destroy(cr);
        return;
    }
    mocha_log("Wallpaper loaded: %dx%d", img_w, img_h);
    rgba_to_cairo_argb32(data, img_w, img_h);
    cairo_surface_t *img_surface = cairo_image_surface_create_for_data(
        data, CAIRO_FORMAT_ARGB32, img_w, img_h, img_w * 4);
    if(cairo_surface_status(img_surface) != CAIRO_STATUS_SUCCESS) {
        mocha_log("Failed to create Cairo surface for wallpaper");
        stbi_image_free(data);
        return;
    }
    cairo_t *cr = cairo_create(surface);
    const char *mode = config.colors.wallpaper_mode[0]
                           ? config.colors.wallpaper_mode
                           : "stretch_fit";
    if(strcmp(mode, "stretch_fit") == 0) {
        cairo_save(cr);
        cairo_scale(cr, (double)width / img_w, (double)height / img_h);
        cairo_set_source_surface(cr, img_surface, 0, 0);
        cairo_paint(cr);
        cairo_restore(cr);
    } else {
        double sx = (double)width / img_w;
        double sy = (double)height / img_h;
        double scale = sx < sy ? sx : sy;
        double dx = (width - img_w * scale) / 2;
        double dy = (height - img_h * scale) / 2;
        cairo_save(cr);
        cairo_translate(cr, dx, dy);
        cairo_scale(cr, scale, scale);
        cairo_set_source_surface(cr, img_surface, 0, 0);
        cairo_paint(cr);
        cairo_restore(cr);
    }
    cairo_destroy(cr);
    cairo_surface_destroy(img_surface);
    stbi_image_free(data);
}
