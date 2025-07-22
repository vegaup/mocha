#include "ui/toast.h"

Toast *toasts = NULL;

void show_toast(const char *message) {
    int toast_x = DisplayWidth(dpy, screen) - TOAST_WIDTH - TOAST_PADDING;
    int toast_y = TOAST_PADDING;

    Toast *existing_toast = toasts;
    while(existing_toast) {
        toast_y += TOAST_HEIGHT + TOAST_PADDING;
        existing_toast = existing_toast->next;
    }

    XSetWindowAttributes attrs;
    attrs.override_redirect = True;
    attrs.background_pixel = panel_color;
    attrs.border_pixel = focus_color;
    attrs.event_mask = ExposureMask;

    Window toast_win = XCreateWindow(
        dpy, root, toast_x, toast_y, TOAST_WIDTH, TOAST_HEIGHT, 1,
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
    new_toast->next = toasts;
    toasts = new_toast;
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
}
