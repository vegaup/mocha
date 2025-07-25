#include "main.h"

#include <X11/XF86keysym.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/shape.h>
#include <X11/keysym.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>
#include <execinfo.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "event/event.h"
#include "features/launcher.h"
#include "ui/toast.h"
#include "util/client.h"
#include "util/config.h"

struct Config config = {0};

Display *dpy;
Window root;
Colormap colormap;
Visual *argb_visual = NULL;
int argb_depth = 0;
Colormap argb_colormap;

int screen;
unsigned long border_color, focus_color, panel_color, foreground_color,
    accent_color;

static Bool check_extension(const char *extname) {
    int n;
    char **extlist = XListExtensions(dpy, &n);
    Bool result = False;

    if(extlist) {
        for(int i = 0; i < n; i++) {
            if(strcmp(extlist[i], extname) == 0) {
                result = True;
                break;
            }
        }
        XFreeExtensionList(extlist);
    }
    return result;
}

static int file_exists(const char *path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

static void setup_mirroring() {
    mocha_log("Attempting to mirror displays using xrandr...");
    const char *cmd = "xrandr --output eDP-1 --same-as DP-1";
    int result = system(cmd);
    if(result == 0) {
        mocha_log("xrandr mirroring command executed successfully.");
    } else {
        mocha_log(
            "xrandr mirroring command failed or xrandr is not installed.");
    }
}

void sigsegv_handler(int sig) {
    void *array[10];
    size_t size;

    size = backtrace(array, 10);
    fprintf(stderr, "Error: signal %d:\\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}

int main(void) {
    signal(SIGSEGV, sigsegv_handler);
    mocha_log("Mocha v1.0 starting...");

    mocha_log("Loading config...");
    const char *home = getenv("HOME");
    if(!home) home = getpwuid(getuid())->pw_dir;
    char config_dir[256];
    snprintf(config_dir, sizeof(config_dir), "%s/.config/mocha", home);

    char path_config[300], path_theme[300], path_keybinds[300],
        path_features[300], path_quotes[300];
    snprintf(path_config, sizeof(path_config), "%s/config.mconf", config_dir);
    snprintf(path_theme, sizeof(path_theme), "%s/theme.mconf", config_dir);
    snprintf(path_keybinds, sizeof(path_keybinds), "%s/keybinds.mconf",
             config_dir);
    snprintf(path_features, sizeof(path_features), "%s/features.mconf",
             config_dir);
    snprintf(path_quotes, sizeof(path_quotes), "%s/quotes.txt", config_dir);

    mocha_log("Attempting to load config from: %s", config_dir);

    load_quotes(path_quotes);

    mocha_log("Opening display...");
    char *display = getenv("DISPLAY");

    if(!display) {
        panic("DISPLAY environment variable not set");
    }

    dpy = XOpenDisplay(NULL);
    if(!dpy) panic("Unable to open X display");

    mocha_log("Setting up error handler...");
    XSetErrorHandler(handleXError);

    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);
    colormap = DefaultColormap(dpy, screen);

    XVisualInfo vinfo;
    if(XMatchVisualInfo(dpy, screen, 32, TrueColor, &vinfo)) {
        argb_visual = vinfo.visual;
        argb_depth = vinfo.depth;
        argb_colormap = XCreateColormap(dpy, root, argb_visual, AllocNone);
    } else {
        mocha_log("No 32-bit visual found, transparency will be disabled.");
        argb_visual = DefaultVisual(dpy, screen);
        argb_depth = DefaultDepth(dpy, screen);
        argb_colormap = colormap;
    }

    mocha_log("Parsing config file: %s", path_config);
    parse_config(path_config, &config);
    mocha_log("Parsing theme file: %s", path_theme);
    parse_config(path_theme, &config);
    mocha_log("Parsing keybinds file: %s", path_keybinds);
    parse_config(path_keybinds, &config);
    mocha_log("Parsing features file: %s", path_features);
    parse_config(path_features, &config);

    mocha_log("Setting up colors...");
    XColor border_xcolor, focus_xcolor, panel_xcolor, foreground_xcolor,
        accent_xcolor;
    if(config.colors.border[0])
        XParseColor(dpy, colormap, config.colors.border, &border_xcolor);
    else
        XParseColor(dpy, colormap, "#6f529eff", &border_xcolor);
    if(config.colors.focus[0])
        XParseColor(dpy, colormap, config.colors.focus, &focus_xcolor);
    else
        XParseColor(dpy, colormap, "#6200ffff", &focus_xcolor);
    if(config.colors.panel[0])
        XParseColor(dpy, colormap, config.colors.panel, &panel_xcolor);
    else
        XParseColor(dpy, colormap, "#222222ff", &panel_xcolor);
    if(config.colors.foreground[0])
        XParseColor(dpy, colormap, config.colors.foreground,
                    &foreground_xcolor);
    else
        XParseColor(dpy, colormap, "#ffffffff", &foreground_xcolor);
    if(config.colors.accent[0])
        XParseColor(dpy, colormap, config.colors.accent, &accent_xcolor);
    else
        XParseColor(dpy, colormap, "#4285F4ff", &accent_xcolor);

    XAllocColor(dpy, colormap, &border_xcolor);
    border_color = border_xcolor.pixel;
    XAllocColor(dpy, colormap, &focus_xcolor);
    focus_color = focus_xcolor.pixel;
    XAllocColor(dpy, colormap, &panel_xcolor);
    panel_color = panel_xcolor.pixel;
    XAllocColor(dpy, colormap, &foreground_xcolor);
    foreground_color = foreground_xcolor.pixel;
    if(!foreground_color)
        mocha_log(
            "Warning: foreground_color allocation failed, text may be black");
    XAllocColor(dpy, colormap, &accent_xcolor);
    accent_color = accent_xcolor.pixel;

    mocha_log("Setting up taskbar...");
    if(config.features.quotes_enabled) show_quote_window(get_random_quote());
    if(config.exec_one[0]) system(config.exec_one);

    int taskbar_height = 60;
    int screen_height = DisplayHeight(dpy, screen);
    int screen_width = DisplayWidth(dpy, screen);
    int root_depth = DefaultDepth(dpy, screen);
    mocha_log("Root window depth: %d", root_depth);

    Pixmap wallpaper_pixmap = None;
    if(config.colors.wallpaper[0]) {
        mocha_log("Creating wallpaper pixmap: %dx%d", screen_width,
                  screen_height);
        wallpaper_pixmap =
            XCreatePixmap(dpy, root, screen_width, screen_height, root_depth);
        if(wallpaper_pixmap == None) {
            mocha_log("Failed to create wallpaper pixmap!");
        } else {
            mocha_log("Drawing wallpaper to pixmap...");
            cairo_surface_t *pixmap_surface = cairo_xlib_surface_create(
                dpy, wallpaper_pixmap, DefaultVisual(dpy, screen), screen_width,
                screen_height);
            if(cairo_surface_status(pixmap_surface) != CAIRO_STATUS_SUCCESS) {
                mocha_log("Failed to create Cairo surface for pixmap!");
            } else {
                mocha_draw_wallpaper(pixmap_surface, config.colors.wallpaper,
                                     screen_width, screen_height);
                mocha_log("Setting wallpaper pixmap as root background...");
                XSetWindowBackgroundPixmap(dpy, root, wallpaper_pixmap);
                mocha_log("Clearing root window to show wallpaper...");
                XClearWindow(dpy, root);
            }
            cairo_surface_destroy(pixmap_surface);
        }
    } else {
        XSetWindowBackground(dpy, root, 0x000000);
        XClearWindow(dpy, root);
    }

    Bool has_render = False;
    int render_event, render_error;
    if(XRenderQueryExtension(dpy, &render_event, &render_error)) {
        has_render = True;
    }

    Window taskbar;
    if(has_render) {
        taskbar =
            XCreateSimpleWindow(dpy, root, 0, screen_height - taskbar_height,
                                screen_width, taskbar_height, 0, 0, 0x000000);
    } else {
        taskbar =
            XCreateSimpleWindow(dpy, root, 0, screen_height - taskbar_height,
                                screen_width, taskbar_height, 0, 0, 0x000000);
    }
    XSelectInput(dpy, taskbar,
                 ExposureMask | ButtonPressMask | ButtonReleaseMask);
    XMapWindow(dpy, taskbar);
    Atom dock_type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
    XChangeProperty(dpy, taskbar,
                    XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False), XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)&dock_type, 1);
    mocha_init_dock();

    mocha_tile_clients(taskbar_height);

    Cursor cursor = XCreateFontCursor(dpy, XC_left_ptr);
    XDefineCursor(dpy, root, cursor);

    XSelectInput(dpy, root,
                 SubstructureRedirectMask | SubstructureNotifyMask |
                     ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                     EnterWindowMask | LeaveWindowMask);

    mocha_log("Setting up keybinds...");
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

    char welcome_lock_path[300];
    snprintf(welcome_lock_path, sizeof(welcome_lock_path),
             "%s/mocha_first_time_welcome_lock", config_dir);
    FILE *lock = fopen(welcome_lock_path, "r");
    if(!lock) {
        show_toast(
            "Welcome to Mocha! Check out our docs here: "
            "https://github.com/thoq-jar/mocha");
        FILE *new_lock = fopen(welcome_lock_path, "w");
        if(new_lock) fclose(new_lock);
    } else {
        fclose(lock);
    }

    mocha_log("Mocha v1.0 started!");
    struct DragState drag_state = {0};

    XEvent event;
    for(;;) {
        cleanup_toasts();
        animate_toasts();
        XNextEvent(dpy, &event);

        mocha_handle_event(event, taskbar, &drag_state, taskbar_height,
                           config.features.tiling_enabled);
    }

    XCloseDisplay(dpy);
    if(wallpaper_pixmap != None) XFreePixmap(dpy, wallpaper_pixmap);
    return 0;
}
