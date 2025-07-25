#include "util/config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "util/config.h"

#define MAX_COLOR_LEN 16
#define MAX_KEYBIND_LEN 32
#define MAX_CMD_LEN 256
#define MAX_BINDS 32
#define MAX_PATH_LEN 256

static char *trim(char *str) {
    while(isspace(*str)) str++;
    char *end = str + strlen(str) - 1;
    while(end > str && isspace(*end)) *end-- = 0;
    return str;
}

int parse_config(const char *filename, struct Config *cfg) {
    FILE *f = fopen(filename, "r");
    if(!f) {
        mocha_log("Config] Failed to open '%s'", filename);
        return 1;
    }
    mocha_log("Config] Parsing '%s'", filename);

    char line[512];
    char current_block[64] = "";
    while(fgets(line, sizeof(line), f)) {
        char *l = trim(line);
        if(*l == 0 || *l == '#') continue;
        if(strchr(l, '{')) {
            sscanf(l, "%31s", current_block);
            continue;
        }
        if(strchr(l, '}')) {
            current_block[0] = 0;
            continue;
        }
        char *eq = strchr(l, '=');
        if(eq) {
            *eq = 0;
            char *k = trim(l);
            char *v = trim(eq + 1);
            if(*v == '"') {
                v++;
                char *q = strchr(v, '"');
                if(q) *q = 0;
            }
            mocha_log("Config] Found: block='%s', key='%s', value='%s'",
                      current_block, k, v);

            if(strcmp(current_block, "colors") == 0) {
                if(strcmp(k, "border") == 0)
                    strncpy(cfg->colors.border, v, MAX_COLOR_LEN);
                else if(strcmp(k, "focus") == 0)
                    strncpy(cfg->colors.focus, v, MAX_COLOR_LEN);
                else if(strcmp(k, "panel") == 0)
                    strncpy(cfg->colors.panel, v, MAX_COLOR_LEN);
                else if(strcmp(k, "accent") == 0)
                    strncpy(cfg->colors.accent, v, MAX_COLOR_LEN);
                else if(strcmp(k, "foreground") == 0)
                    strncpy(cfg->colors.foreground, v, MAX_COLOR_LEN);
                else if(strcmp(k, "wallpaper") == 0)
                    strncpy(cfg->colors.wallpaper, v, MAX_PATH_LEN);
            } else if(strcmp(current_block, "wallpaper") == 0) {
                if(strcmp(k, "image") == 0)
                    strncpy(cfg->colors.wallpaper, v, MAX_PATH_LEN);
                else if(strcmp(k, "mode") == 0)
                    strncpy(cfg->colors.wallpaper_mode, v, 32);
            } else if(strcmp(current_block, "keybinds") == 0) {
                if(cfg->num_keybinds < MAX_BINDS) {
                    strncpy(cfg->keybinds[cfg->num_keybinds].key, k,
                            MAX_KEYBIND_LEN);
                    strncpy(cfg->keybinds[cfg->num_keybinds].action, v,
                            MAX_CMD_LEN);
                    cfg->num_keybinds++;
                }
            } else {
                if(strcmp(k, "launcher-command") == 0)
                    strncpy(cfg->launcher_cmd, v, MAX_CMD_LEN);
                else if(strcmp(k, "exec-one") == 0)
                    strncpy(cfg->exec_one, v, MAX_CMD_LEN);
                else if(strcmp(k, "tiling_enabled") == 0)
                    cfg->features.tiling_enabled = atoi(v);
                else if(strcmp(k, "quotes_enabled") == 0)
                    cfg->features.quotes_enabled = atoi(v);
                else if(strcmp(k, "border_radius") == 0)
                    cfg->features.border_radius = atoi(v);
                else if(strcmp(k, "wallpaper") == 0)
                    strncpy(cfg->colors.wallpaper, v, MAX_PATH_LEN);
            }
        }
    }
    fclose(f);
    return 0;
}

int get_border_width() { return 5; }