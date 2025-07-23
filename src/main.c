#include "main.h"

#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include "event/event.h"
#include "features/launcher.h"
#include "ui/toast.h"
#include "util/client.h"
#include "util/config.h"

struct Config config = {0};

Display *dpy;
Window root;
Colormap colormap;

int screen;
unsigned long border_color, focus_color, panel_color, foreground_color, accent_color;

int main(void) {
    const char *home = getenv("HOME");
    if (!home) home = getpwuid(getuid())->pw_dir;
    char config_dir[256];
    snprintf(config_dir, sizeof(config_dir), "%s/.config/mocha", home);

    char path_config[300], path_theme[300], path_keybinds[300], path_features[300];
    snprintf(path_config, sizeof(path_config), "%s/config.mconf", config_dir);
    snprintf(path_theme, sizeof(path_theme), "%s/theme.mconf", config_dir);
    snprintf(path_keybinds, sizeof(path_keybinds), "%s/keybinds.mconf", config_dir);
    snprintf(path_features, sizeof(path_features), "%s/features.mconf", config_dir);

    mocha_log("Mocha v1.0 starting...");

    char *display = getenv("DISPLAY");
    if(!display) panic("DISPLAY environment variable not set");

    dpy = XOpenDisplay(NULL);
    if(!dpy) panic("Unable to open X display");

    XSetErrorHandler(handleXError);

    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);
    colormap = DefaultColormap(dpy, screen);

    parse_config(path_config, &config);    // general/top-level
    parse_config(path_theme, &config);     // colors
    parse_config(path_keybinds, &config);  // keybinds
    parse_config(path_features, &config);  // features

    XColor border_xcolor, focus_xcolor, panel_xcolor, foreground_xcolor, accent_xcolor;
    if(config.colors.border[0]) XParseColor(dpy, colormap, config.colors.border, &border_xcolor);
    else XParseColor(dpy, colormap, "#6f529eff", &border_xcolor);
    if(config.colors.focus[0]) XParseColor(dpy, colormap, config.colors.focus, &focus_xcolor);
    else XParseColor(dpy, colormap, "#6200ffff", &focus_xcolor);
    if(config.colors.panel[0]) XParseColor(dpy, colormap, config.colors.panel, &panel_xcolor);
    else XParseColor(dpy, colormap, "#969696ff", &panel_xcolor);
    if(config.colors.foreground[0]) XParseColor(dpy, colormap, config.colors.foreground, &foreground_xcolor);
    else XParseColor(dpy, colormap, "#ffffffff", &foreground_xcolor);
    if(config.colors.accent[0]) XParseColor(dpy, colormap, config.colors.accent, &accent_xcolor);
    else XParseColor(dpy, colormap, "#7300ffff", &accent_xcolor);

    XAllocColor(dpy, colormap, &border_xcolor); border_color = border_xcolor.pixel;
    XAllocColor(dpy, colormap, &focus_xcolor); focus_color = focus_xcolor.pixel;
    XAllocColor(dpy, colormap, &panel_xcolor); panel_color = panel_xcolor.pixel;
    XAllocColor(dpy, colormap, &foreground_xcolor); foreground_color = foreground_xcolor.pixel;
    if(!foreground_color) mocha_log("Warning: foreground_color allocation failed, text may be black");
    XAllocColor(dpy, colormap, &accent_xcolor); accent_color = accent_xcolor.pixel;

    if(config.exec_one[0]) system(config.exec_one);

    int taskbar_height = 40;
    XSetWindowAttributes taskbar_attrs;
    taskbar_attrs.background_pixel = panel_color;
    Window taskbar =
        XCreateWindow(dpy, root, 0, 0, DisplayWidth(dpy, screen),
                      taskbar_height, 0, CopyFromParent, InputOutput,
                      CopyFromParent, CWBackPixel, &taskbar_attrs);

    XSelectInput(dpy, taskbar, ExposureMask | ButtonPressMask);
    XMapWindow(dpy, taskbar);
    XRaiseWindow(dpy, taskbar);

    Cursor cursor = XCreateFontCursor(dpy, XC_left_ptr);
    XDefineCursor(dpy, root, cursor);
    XSync(dpy, False);

    XSelectInput(dpy, root,
                 SubstructureRedirectMask | SubstructureNotifyMask |
                     ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                     EnterWindowMask | LeaveWindowMask);

    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_z), Mod1Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_0), Mod1Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_f), Mod1Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_space), Mod1Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_q), Mod1Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_c), Mod1Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XF86XK_AudioRaiseVolume), 0, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XF86XK_AudioLowerVolume), 0, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XF86XK_AudioMute), 0, root, True,
             GrabModeAsync, GrabModeAsync);

    Window active_window = 0;
    int dragging = 0, resizing = 0;
    int start_x = 0, start_y = 0;
    int win_x = 0, win_y = 0, win_w = 0, win_h = 0;

    char welcome_lock_path[300];
    snprintf(welcome_lock_path, sizeof(welcome_lock_path), "%s/mocha_first_time_welcome_lock", config_dir);
    FILE *lock = fopen(welcome_lock_path, "r");
    if (!lock) {
        show_toast("Welcome to Mocha! Check out our docs here: https://github.com/thoq-jar/mocha");
        FILE *new_lock = fopen(welcome_lock_path, "w");
        if (new_lock) fclose(new_lock);
    } else {
        fclose(lock);
    }

    struct DragState drag_state = {0};

    XEvent event;
    for(;;) {
        cleanup_toasts();
        animate_toasts();
        XNextEvent(dpy, &event);

        mocha_handle_event(event, taskbar, &drag_state, taskbar_height, config.tiling_enabled);
    }

    XCloseDisplay(dpy);
    return 0;
}
