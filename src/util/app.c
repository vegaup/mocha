#include "util/app.h"

#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lib/stb_image.h"
#include "main.h"
#include "util/config.h"

AppInfo apps[MAX_APPS];
int app_count = 0;
bool apps_loaded = false;

static void cleanup_exec_command(char *exec) {
    for(char *p = exec; *p; p++) {
        if(*p == '%') {
            *p = ' ';
            if(*(p + 1) != '\0') {
                *(p + 1) = ' ';
            }
        }
    }
}

void parse_desktop_file(const char *path, AppInfo *info) {
    FILE *file = fopen(path, "r");
    if(!file) return;

    char line[512];
    info->icon_surface = NULL;
    while(fgets(line, sizeof(line), file)) {
        if(strncmp(line, "Name=", 5) == 0) {
            sscanf(line, "Name=%[^\n]", info->name);
        } else if(strncmp(line, "Exec=", 5) == 0) {
            sscanf(line, "Exec=%[^\n]", info->exec);
            cleanup_exec_command(info->exec);
        } else if(strncmp(line, "Icon=", 5) == 0) {
            sscanf(line, "Icon=%[^\n]", info->icon);
        } else if(strncmp(line, "StartupWMClass=", 15) == 0) {
            sscanf(line, "StartupWMClass=%[^\n]", info->wm_class);
        } else if(strncmp(line, "NoDisplay=", 10) == 0) {
            if(strstr(line, "true")) {
                info->name[0] = '\0';
            }
        }
    }
    fclose(file);
}

static char *find_icon_path(const char *icon_name, int size) {
    if(!icon_name || !icon_name[0]) return NULL;

    char *path = malloc(1024);
    const char *icon_themes[] = {"/usr/share/icons/hicolor",
                                 "/usr/share/pixmaps", NULL};
    const char *extensions[] = {".png", NULL};

    if(icon_name[0] == '/' && access(icon_name, F_OK) == 0) {
        strncpy(path, icon_name, 1023);
        return path;
    }

    for(int t = 0; icon_themes[t]; t++) {
        snprintf(path, 1024, "%s/%dx%d/apps/%s%s", icon_themes[t], size, size,
                 icon_name, extensions[0]);
        if(access(path, F_OK) == 0) return path;
        snprintf(path, 1024, "%s/apps/%d/%s%s", icon_themes[t], size, icon_name,
                 extensions[0]);
        if(access(path, F_OK) == 0) return path;
        snprintf(path, 1024, "%s/%s%s", icon_themes[t], icon_name,
                 extensions[0]);
        if(access(path, F_OK) == 0) return path;
    }

    snprintf(path, 1024, "/usr/share/pixmaps/%s%s", icon_name, extensions[0]);
    if(access(path, F_OK) == 0) return path;

    free(path);
    return NULL;
}

static void rgba_to_cairo_argb32(unsigned char *data, int width, int height) {
    for(int i = 0; i < width * height; ++i) {
        uint8_t r = data[i * 4 + 0];
        uint8_t g = data[i * 4 + 1];
        uint8_t b = data[i * 4 + 2];
        uint8_t a = data[i * 4 + 3];
        double alpha = a / 255.0;
        uint32_t r_pre = r * alpha;
        uint32_t g_pre = g * alpha;
        uint32_t b_pre = b * alpha;
        ((uint32_t *)data)[i] =
            (a << 24) | (r_pre << 16) | (g_pre << 8) | b_pre;
    }
}

void load_app_icon(AppInfo *app, int size) {
    if(app->icon_surface) return;

    char *icon_path = find_icon_path(app->icon, size);
    if(!icon_path) {
        return;
    }

    int w, h, n;
    unsigned char *data = stbi_load(icon_path, &w, &h, &n, 4);
    if(!data) {
        free(icon_path);
        return;
    }

    rgba_to_cairo_argb32(data, w, h);
    cairo_surface_t *icon_surface_full = cairo_image_surface_create_for_data(
        data, CAIRO_FORMAT_ARGB32, w, h, w * 4);

    app->icon_surface =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
    cairo_t *cr = cairo_create(app->icon_surface);

    cairo_scale(cr, (double)size / w, (double)size / h);
    cairo_set_source_surface(cr, icon_surface_full, 0, 0);
    cairo_paint(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(icon_surface_full);
    stbi_image_free(data);
    free(icon_path);
}

void find_applications() {
    if(apps_loaded) return;
    const char *app_dirs[] = {"/usr/share/applications",
                              "/usr/local/share/applications",
                              "~/.local/share/applications", NULL};
    app_count = 0;

    for(int i = 0; app_dirs[i] != NULL; i++) {
        const char *dir_path = app_dirs[i];
        if(dir_path[0] == '~') {
            const char *home = getenv("HOME");
            if(home) {
                char path[1024];
                snprintf(path, sizeof(path), "%s%s", home, dir_path + 1);
                dir_path = strdup(path);
            } else {
                continue;
            }
        }

        DIR *dir = opendir(dir_path);
        if(!dir) {
            if(app_dirs[i][0] == '~') free((void *)dir_path);
            continue;
        }

        struct dirent *entry;
        while((entry = readdir(dir)) != NULL && app_count < MAX_APPS) {
            if(strstr(entry->d_name, ".desktop")) {
                char path[1024];
                snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);
                parse_desktop_file(path, &apps[app_count]);
                if(apps[app_count].name[0] && apps[app_count].exec[0]) {
                    app_count++;
                }
            }
        }
        closedir(dir);
        if(app_dirs[i][0] == '~') free((void *)dir_path);
    }
    apps_loaded = true;
}

AppInfo *find_app_by_wmclass(const char *wm_class) {
    if(!wm_class) return NULL;
    for(int i = 0; i < app_count; ++i) {
        if(apps[i].wm_class[0] && strcmp(apps[i].wm_class, wm_class) == 0) {
            return &apps[i];
        }
    }
    return NULL;
}