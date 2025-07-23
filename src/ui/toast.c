#include "ui/toast.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include "../util/config.h"
#include <math.h>

Toast *toasts = NULL;

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
    int size = 200; // square size
    int x = (DisplayWidth(dpy, screen) - size) / 2;
    int y = (DisplayHeight(dpy, screen) - size) / 2;

    if (status_toast_ptr) {
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
        dpy, root, x, y, size, size, 1,
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
    while (t) {
        XMoveWindow(dpy, t->win, t->current_x, toast_y);
        toast_y += toast_h + toast_pad;
        t = t->next;
    }
    if (status_toast_ptr) {
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
    if (status_toast_ptr && difftime(now, status_toast_ptr->created_at) > TOAST_TIMEOUT) {
        XDestroyWindow(dpy, status_toast_ptr->win);
        free(status_toast_ptr);
        status_toast_ptr = NULL;
    }
}
