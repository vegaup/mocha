#ifndef MOCHA_CONFIG_H
#define MOCHA_CONFIG_H

#define MAX_COLOR_LEN 16
#define MAX_KEYBIND_LEN 32
#define MAX_CMD_LEN 256
#define MAX_BINDS 32
#define MAX_PATH_LEN 256

struct ConfigColors {
    char border[MAX_COLOR_LEN];
    char focus[MAX_COLOR_LEN];
    char panel[MAX_COLOR_LEN];
    char foreground[MAX_COLOR_LEN];
    char accent[MAX_COLOR_LEN];
    char wallpaper[MAX_PATH_LEN];
    char wallpaper_mode[32];
};

struct ConfigKeybind {
    char key[MAX_KEYBIND_LEN];
    char action[MAX_CMD_LEN];
};

struct ConfigFeatures {
    int tiling_enabled;
    int quotes_enabled;
    int border_radius;
};

struct Config {
    struct ConfigColors colors;
    struct ConfigKeybind keybinds[MAX_BINDS];
    struct ConfigFeatures features;
    int num_keybinds;
    char launcher_cmd[MAX_CMD_LEN];
    char exec_one[MAX_CMD_LEN];
};

extern struct Config config;

int parse_config(const char *filename, struct Config *cfg);

#endif  // MOCHA_CONFIG_H