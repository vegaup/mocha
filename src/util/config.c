#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_COLOR_LEN 16
#define MAX_KEYBIND_LEN 32
#define MAX_CMD_LEN 256
#define MAX_BINDS 32

struct ConfigColors {
    char border[MAX_COLOR_LEN];
    char focus[MAX_COLOR_LEN];
    char panel[MAX_COLOR_LEN];
};

struct ConfigKeybind {
    char key[MAX_KEYBIND_LEN];
    char action[MAX_CMD_LEN];
};

struct Config {
    struct ConfigColors colors;
    struct ConfigKeybind keybinds[MAX_BINDS];
    int num_keybinds;
    char launcher_cmd[MAX_CMD_LEN];
    char exec_one[MAX_CMD_LEN];
    int tiling_enabled;
};

static char *trim(char *str) {
    while(isspace(*str)) str++;
    char *end = str + strlen(str) - 1;
    while(end > str && isspace(*end)) *end-- = 0;
    return str;
}

int parse_config(const char *filename, struct Config *cfg) {
    FILE *f = fopen(filename, "r");
    if(!f) return -1;
    char line[512];
    char current_block[32] = "";
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
            if(strcmp(current_block, "colors") == 0) {
                if(strcmp(k, "border") == 0) strncpy(cfg->colors.border, v, MAX_COLOR_LEN);
                else if(strcmp(k, "focus") == 0) strncpy(cfg->colors.focus, v, MAX_COLOR_LEN);
                else if(strcmp(k, "panel") == 0) strncpy(cfg->colors.panel, v, MAX_COLOR_LEN);
            } else if(strcmp(current_block, "keybinds") == 0) {
                if(cfg->num_keybinds < MAX_BINDS) {
                    strncpy(cfg->keybinds[cfg->num_keybinds].key, k, MAX_KEYBIND_LEN);
                    strncpy(cfg->keybinds[cfg->num_keybinds].action, v, MAX_CMD_LEN);
                    cfg->num_keybinds++;
                }
            } else if(strcmp(current_block, "features") == 0) {
                if(strcmp(k, "tiling") == 0) cfg->tiling_enabled = (strcmp(v, "true") == 0 || strcmp(v, "1") == 0);
            } else {
                if(strcmp(k, "launcher-command") == 0) strncpy(cfg->launcher_cmd, v, MAX_CMD_LEN);
                else if(strcmp(k, "exec-one") == 0) strncpy(cfg->exec_one, v, MAX_CMD_LEN);
                else if(strcmp(k, "tiling") == 0) cfg->tiling_enabled = (strcmp(v, "true") == 0 || strcmp(v, "1") == 0);
            }
        }
    }
    fclose(f);
    return 0;
} 