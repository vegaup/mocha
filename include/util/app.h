#ifndef APP_H
#define APP_H

#include <cairo/cairo.h>
#include <stdbool.h>

#define MAX_APPS 512

typedef struct {
  char name[256];
  char exec[256];
  char icon[256];
  char wm_class[256];
  cairo_surface_t *icon_surface;
} AppInfo;

extern AppInfo apps[MAX_APPS];
extern int app_count;
extern bool apps_loaded;

void find_applications();
AppInfo *find_app_by_wmclass(const char *wm_class);
void load_app_icon(AppInfo *app, int size);

#endif // APP_H