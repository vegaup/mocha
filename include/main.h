#ifndef MAIN_H
#define MAIN_H

#include <X11/Xlib.h>
#include <stdio.h>

#define AltMask Mod1Mask  // 8
#define BORDER_WIDTH 5

/* X11 connection */
extern Display *dpy;
/* Display's root */
extern Window root;
/* Colors */
extern unsigned long border_color, focus_color, panel_color, foreground_color, accent_color;

void grabKey(KeySym sym, unsigned int mod);
void panic(char *msg);
void mocha_log(const char *fmt, ...);
void mocha_error(FILE *stream, const char *fmt, ...);
void mocha_shutdown();
int handleXError(Display *dpy, XErrorEvent *error);

#endif  // MAIN_H