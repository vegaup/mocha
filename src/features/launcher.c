#include "features/launcher.h"

#include <stdio.h>
#include <stdlib.h>

#include "mocha_launcher.h"

extern Display* dpy;
extern int screen;

void mocha_launch_menu() { show_launcher(dpy, screen); }
