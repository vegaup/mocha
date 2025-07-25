#include "ui/toast.h"

#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>
#include <math.h>
#include <time.h>

#include "util/config.h"

#define MAX_QUOTES 64
#define MAX_QUOTE_LEN 256
Window quote_win = 0;

static char quotes[MAX_QUOTES][MAX_QUOTE_LEN];
static char current_quote[MAX_QUOTE_LEN] = "";
static int num_quotes = 0;
static int quote_width = 1440;

Toast *toasts = NULL;

void load_quotes(const char *filename) {
    num_quotes = 0;
    FILE *f = fopen(filename, "r");
    if(!f) return;
    char buf[MAX_QUOTE_LEN];
    while(fgets(buf, sizeof(buf), f) && num_quotes < MAX_QUOTES) {
        size_t len = strlen(buf);
        if(len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
        strncpy(quotes[num_quotes], buf, MAX_QUOTE_LEN - 1);
        quotes[num_quotes][MAX_QUOTE_LEN - 1] = '\0';
        num_quotes++;
    }
    fclose(f);
}

const char *get_random_quote() {
    if(num_quotes == 0) return "";
    srand(time(NULL));
    int idx = rand() % num_quotes;
    return quotes[idx];
}

void show_quote_window(const char *quote) {
    if(quote_win) destroy_quote_window();
    int w = quote_width;
    int h = 48;
    int x = (DisplayWidth(dpy, screen) - w) / 2;
    int y = DisplayHeight(dpy, screen) - h - 40;

    Pixmap bg_pixmap =
        XCreatePixmap(dpy, root, w, h, DefaultDepth(dpy, screen));
    GC gc = XCreateGC(dpy, root, 0, NULL);
    XCopyArea(dpy, root, bg_pixmap, gc, x, y, w, h, 0, 0);
    XFreeGC(dpy, gc);

    XSetWindowAttributes attrs;
    attrs.override_redirect = True;
    attrs.background_pixmap = bg_pixmap;
    attrs.border_pixel = 0;
    attrs.event_mask = ExposureMask;

    quote_win = XCreateWindow(
        dpy, root, x, y, w, h, 0, DefaultDepth(dpy, screen), CopyFromParent,
        DefaultVisual(dpy, screen),
        CWOverrideRedirect | CWBackPixmap | CWBorderPixel | CWEventMask,
        &attrs);

    XMapWindow(dpy, quote_win);
    XFreePixmap(dpy, bg_pixmap);

    strncpy(current_quote, quote, MAX_QUOTE_LEN - 1);
    current_quote[MAX_QUOTE_LEN - 1] = '\0';
}

void update_quote_window(const char *quote) {
    if(!quote_win) return;
    strncpy(current_quote, quote, MAX_QUOTE_LEN - 1);
    current_quote[MAX_QUOTE_LEN - 1] = '\0';
    XClearWindow(dpy, quote_win);
}

void destroy_quote_window() {
    if(quote_win) {
        XDestroyWindow(dpy, quote_win);
        quote_win = 0;
    }
}

void draw_quote_window() {
    if(!quote_win) return;
    int w = quote_width;
    int h = 48;

    cairo_surface_t *surface = cairo_xlib_surface_create(
        dpy, quote_win, DefaultVisual(dpy, screen), w, h);
    cairo_t *cr = cairo_create(surface);

    cairo_set_source_rgba(cr, 0, 0, 0, 0.5);
    cairo_paint(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    XftDraw *draw = XftDrawCreate(dpy, quote_win, DefaultVisual(dpy, screen),
                                  DefaultColormap(dpy, screen));
    if(!draw) return;

    int font_size = 14;
    char font_pattern[128];
    snprintf(font_pattern, sizeof(font_pattern), "DejaVuSansMono:size=%d",
             font_size);
    XftFont *font = XftFontOpenName(dpy, screen, font_pattern);
    if(!font) {
        snprintf(font_pattern, sizeof(font_pattern), "sans:size=%d", font_size);
        font = XftFontOpenName(dpy, screen, font_pattern);
    }
    if(!font) {
        XftDrawDestroy(draw);
        return;
    }
    XftColor xft_fg;
    XRenderColor render_fg = {
        .red = 0xFFFF, .green = 0xFFFF, .blue = 0xFFFF, .alpha = 0xFFFF};
    if(!XftColorAllocValue(dpy, DefaultVisual(dpy, screen),
                           DefaultColormap(dpy, screen), &render_fg, &xft_fg)) {
        xft_fg.color.red = 0xFFFF;
        xft_fg.color.green = 0xFFFF;
        xft_fg.color.blue = 0xFFFF;
        xft_fg.color.alpha = 0xFFFF;
    }
    int y_offset = 20;
    XGlyphInfo extents;
    XftTextExtentsUtf8(dpy, font, (FcChar8 *)current_quote,
                       strlen(current_quote), &extents);
    int win_width = quote_width;
    int x_offset = (win_width - extents.width) / 2;
    if(x_offset < 0) x_offset = 0;
    XftDrawStringUtf8(draw, &xft_fg, font, x_offset, y_offset,
                      (FcChar8 *)current_quote, strlen(current_quote));
    XftFontClose(dpy, font);
    XftDrawDestroy(draw);
    if(colormap != DefaultColormap(dpy, screen)) {
        XFreeColormap(dpy, colormap);
    }
}

void show_toast(const char *message) {
    int toast_w = TOAST_WIDTH;
    int toast_h = TOAST_HEIGHT;
    int toast_pad = TOAST_PADDING;
    int toast_x = DisplayWidth(dpy, screen) - toast_w - toast_pad;
    int toast_y = toast_pad;

    Toast *existing_toast = toasts;
    while(existing_toast) {
        toast_y += toast_h + toast_pad;
        existing_toast = existing_toast->next;
    }

    XSetWindowAttributes attrs;
    attrs.override_redirect = True;
    attrs.background_pixel = panel_color;
    attrs.border_pixel = focus_color;
    attrs.event_mask = ExposureMask;

    int start_x = toast_x;
    Window toast_win = XCreateWindow(
        dpy, root, start_x, toast_y, toast_w, toast_h, 1,
        DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
        CWOverrideRedirect | CWBackPixel | CWBorderPixel | CWEventMask, &attrs);

    XMapWindow(dpy, toast_win);

    Toast *new_toast = (Toast *)malloc(sizeof(Toast));
    if(!new_toast) {
        perror("malloc");
        return;
    }
    new_toast->win = toast_win;
    strncpy(new_toast->message, message, sizeof(new_toast->message) - 1);
    new_toast->message[sizeof(new_toast->message) - 1] = '\0';
    new_toast->created_at = time(NULL);
    new_toast->current_x = start_x;
    new_toast->target_x = toast_x;
    new_toast->velocity = 0;
    new_toast->alpha = 255;
    new_toast->next = toasts;
    toasts = new_toast;
}

Toast *status_toast_ptr = NULL;

void status_toast(const char *message) {
    int size = 200;
    int x = (DisplayWidth(dpy, screen) - size) / 2;
    int y = (DisplayHeight(dpy, screen) - size) / 2;

    if(status_toast_ptr) {
        XDestroyWindow(dpy, status_toast_ptr->win);
        free(status_toast_ptr);
        status_toast_ptr = NULL;
    }

    XSetWindowAttributes attrs;
    attrs.override_redirect = True;
    attrs.background_pixel = panel_color;
    attrs.border_pixel = focus_color;
    attrs.event_mask = ExposureMask;

    Window toast_win = XCreateWindow(
        dpy, root, x, y, size, size, 1, DefaultDepth(dpy, screen),
        CopyFromParent, DefaultVisual(dpy, screen),
        CWOverrideRedirect | CWBackPixel | CWBorderPixel | CWEventMask, &attrs);

    XMapWindow(dpy, toast_win);

    Toast *new_toast = (Toast *)malloc(sizeof(Toast));
    if(!new_toast) {
        perror("malloc");
        return;
    }
    new_toast->win = toast_win;
    strncpy(new_toast->message, message, sizeof(new_toast->message) - 1);
    new_toast->message[sizeof(new_toast->message) - 1] = '\0';
    new_toast->created_at = time(NULL);
    new_toast->current_x = x;
    new_toast->target_x = x;
    new_toast->velocity = 0;
    new_toast->alpha = 255;
    new_toast->next = NULL;
    status_toast_ptr = new_toast;
}

void animate_toasts() {
    Toast *t = toasts;
    int toast_pad = TOAST_PADDING;
    int toast_h = TOAST_HEIGHT;
    int toast_y = toast_pad;
    while(t) {
        XMoveWindow(dpy, t->win, t->current_x, toast_y);
        toast_y += toast_h + toast_pad;
        t = t->next;
    }
    if(status_toast_ptr) {
        int size = 200;
        int x = (DisplayWidth(dpy, screen) - size) / 2;
        int y = (DisplayHeight(dpy, screen) - size) / 2;
        XMoveWindow(dpy, status_toast_ptr->win, x, y);
    }
}

void cleanup_toasts() {
    time_t now = time(NULL);
    Toast **t = &toasts;
    while(*t) {
        if(difftime(now, (*t)->created_at) > TOAST_TIMEOUT) {
            Toast *to_remove = *t;
            *t = to_remove->next;
            XDestroyWindow(dpy, to_remove->win);
            free(to_remove);
        } else {
            t = &(*t)->next;
        }
    }
    if(status_toast_ptr &&
       difftime(now, status_toast_ptr->created_at) > TOAST_TIMEOUT) {
        XDestroyWindow(dpy, status_toast_ptr->win);
        free(status_toast_ptr);
        status_toast_ptr = NULL;
    }
}
