#define _GNU_SOURCE
#include "rom_scanner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
    const char *folder;
    const char *tag;
    const char *emu_standalone; /* Preferred standalone community emulator (supports full in-game SRAM saves) */
    const char *emu_script;     /* GMenuNX stock script fallback */
    const char *emu_stock;      /* Stock firmware binary fallback */
} FolderMapping;

static const FolderMapping g_mappings[] = {
    {"GB",      "[GB]",  "/mnt/SDCARD/Apps/gambatte/gambatte-dms",   "/mnt/SDCARD/Apps/gmenunx/stockemulators/gamebatte.sh", "/usr/trimui/bin/gamebatte"},
    {"GBA",     "[GBA]", "/mnt/SDCARD/Apps/gpsp/gpsp",               "/mnt/SDCARD/Apps/gmenunx/stockemulators/gpsp.sh",      "/usr/trimui/bin/gpsp"},
    {"FC",      "[FC]",  "/mnt/SDCARD/Apps/fceux/fceux.dge",         "/mnt/SDCARD/Apps/gmenunx/stockemulators/fceux.sh",     "/usr/trimui/bin/fceux"},
    {"SFC",     "[SFC]", "/mnt/SDCARD/Apps/snes9x2002/snes9x2002",   "/mnt/SDCARD/Apps/gmenunx/stockemulators/snes9x4d.sh",  "/usr/trimui/bin/snes9x4d"},
    {"MD",      "[MD]",  "/mnt/SDCARD/Apps/picodrive/PicoDrive",     "/mnt/SDCARD/Apps/gmenunx/stockemulators/PicoDrive.sh", "/usr/trimui/bin/PicoDrive"},
    {"PCE",     "[PCE]", "/mnt/SDCARD/Apps/temper/temper",           "/mnt/SDCARD/Apps/gmenunx/stockemulators/temper.sh",    "/usr/trimui/bin/temper"},
    {"PS",      "[PS]",  "/mnt/SDCARD/Apps/pcsx/pcsx",               "/mnt/SDCARD/Apps/gmenunx/stockemulators/pcsx.sh",      "/usr/trimui/bin/pcsx"},
    {"NGP",     "[NGP]", "/mnt/SDCARD/Apps/race-od/race-od",         "/mnt/SDCARD/Apps/gmenunx/stockemulators/gngeo.sh",     "/usr/trimui/bin/gngeo"},
    {"ARCADE",  "[ARC]", "/mnt/SDCARD/Apps/mame4all/mame4all",       "/mnt/SDCARD/Apps/gmenunx/stockemulators/mame4allx.sh", "/usr/trimui/bin/mame4allx"},
    {"NEOGEO",  "[NEO]", "/mnt/SDCARD/Apps/gngeo/gngeo",             "/mnt/SDCARD/Apps/gmenunx/stockemulators/gngeo.sh",     "/usr/trimui/bin/gngeo"}
};
#define MAPPING_COUNT (sizeof(g_mappings) / sizeof(g_mappings[0]))

/* Clean No-Intro ROM filename into crisp uppercase title */
static void clean_rom_title(const char *filename, char *out, size_t max_len) {
    if (!filename || !out || max_len == 0) return;

    char buf[256];
    strncpy(buf, filename, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    /* 1. Strip extensions */
    const char *exts[] = {
        ".gb.zip", ".zip", ".gbc", ".gba", ".nes", ".sfc", ".smc",
        ".bin", ".pce", ".ngc", ".p8", ".gen", ".smd", ".iso", ".chd", NULL
    };

    for (int i = 0; exts[i] != NULL; ++i) {
        size_t elen = strlen(exts[i]);
        size_t blen = strlen(buf);
        if (blen > elen && strcasecmp(buf + blen - elen, exts[i]) == 0) {
            buf[blen - elen] = '\0';
            break;
        }
    }

    /* 2. Strip bracketed country/revision groups like (Japan) or [!] */
    char *paren = strchr(buf, '(');
    if (paren && paren != buf) {
        *paren = '\0';
    }
    char *bracket = strchr(buf, '[');
    if (bracket && bracket != buf) {
        *bracket = '\0';
    }

    /* 3. Replace underscores with spaces */
    for (int i = 0; buf[i] != '\0'; ++i) {
        if (buf[i] == '_') buf[i] = ' ';
    }

    /* 4. Trim trailing whitespace */
    int len = (int)strlen(buf);
    while (len > 0 && isspace((unsigned char)buf[len - 1])) {
        buf[len - 1] = '\0';
        len--;
    }

    /* 5. Convert to uppercase for Teenage Engineering monospaced look */
    for (int i = 0; buf[i] != '\0'; ++i) {
        buf[i] = (char)toupper((unsigned char)buf[i]);
    }

    strncpy(out, buf, max_len - 1);
    out[max_len - 1] = '\0';
}

static int is_valid_rom_file(const char *filename) {
    if (!filename || filename[0] == '.') return 0;
    if (strcasestr(filename, "COPYING") || strcasestr(filename, "LICENSE") ||
        strcasestr(filename, "PLACE ")) return 0;

    const char *valid_exts[] = {
        ".zip", ".gb", ".gbc", ".gba", ".nes", ".sfc", ".smc",
        ".bin", ".pce", ".ngc", ".p8", ".gen", ".smd", NULL
    };

    size_t flen = strlen(filename);
    for (int i = 0; valid_exts[i] != NULL; ++i) {
        size_t elen = strlen(valid_exts[i]);
        if (flen > elen && strcasecmp(filename + flen - elen, valid_exts[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int rom_scanner_scan(const char *roms_dir, ScannedGame *out_games, int max_games) {
    if (!roms_dir || !out_games || max_games <= 0) return 0;

    int total_games = 0;

    for (size_t m = 0; m < MAPPING_COUNT && total_games < max_games; ++m) {
        char subfolder_path[512];
        snprintf(subfolder_path, sizeof(subfolder_path), "%s/%s", roms_dir, g_mappings[m].folder);

        DIR *dir = opendir(subfolder_path);
        if (!dir) continue;

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && total_games < max_games) {
            if (!is_valid_rom_file(entry->d_name)) continue;

            ScannedGame *g = &out_games[total_games];
            strncpy(g->tag, g_mappings[m].tag, sizeof(g->tag) - 1);
            g->tag[sizeof(g->tag) - 1] = '\0';

            clean_rom_title(entry->d_name, g->title, sizeof(g->title));

            char rom_full_path[512];
            snprintf(rom_full_path, sizeof(rom_full_path), "%s/%s", subfolder_path, entry->d_name);

            /* Prioritize standalone community emulators for proper SRAM in-game saves */
            if (access(g_mappings[m].emu_standalone, F_OK) == 0) {
                snprintf(g->cmd, sizeof(g->cmd), "%s \"%s\"", g_mappings[m].emu_standalone, rom_full_path);
            } else if (access(g_mappings[m].emu_script, F_OK) == 0) {
                snprintf(g->cmd, sizeof(g->cmd), "%s \"%s\"", g_mappings[m].emu_script, rom_full_path);
            } else {
                snprintf(g->cmd, sizeof(g->cmd), "%s \"%s\"", g_mappings[m].emu_stock, rom_full_path);
            }

            total_games++;
        }
        closedir(dir);
    }

    return total_games;
}

void rom_scanner_load_defaults(ScannedGame *out_games, int *out_count) {
    if (!out_games || !out_count) return;

    static const struct {
        const char *tag;
        const char *title;
        const char *cmd;
    } defaults[] = {
        {"[GB]",   "TETRIS",                 "/mnt/SDCARD/Apps/gambatte/gambatte-dms \"/mnt/SDCARD/Roms/GB/tetris.gb\""},
        {"[FC]",   "SUPER MARIO BROS.",      "/mnt/SDCARD/Apps/fceux/fceux.dge \"/mnt/SDCARD/Roms/FC/mario.nes\""},
        {"[GBA]",  "THE LEGEND OF ZELDA",    "/mnt/SDCARD/Apps/gpsp/gpsp \"/mnt/SDCARD/Roms/GBA/zelda.gba\""},
        {"[GB]",   "POKEMON RED",            "/mnt/SDCARD/Apps/gambatte/gambatte-dms \"/mnt/SDCARD/Roms/GB/pokemon.gb\""},
        {"[NGPC]", "METAL SLUG 1ST MISSION", "/mnt/SDCARD/Apps/race-od/race-od \"/mnt/SDCARD/Roms/NGP/mslug.ngc\""},
        {"[FC]",   "GHOSTS 'N GOBLINS",      "/mnt/SDCARD/Apps/fceux/fceux.dge \"/mnt/SDCARD/Roms/FC/makaimura.nes\""}
    };
    int count = sizeof(defaults) / sizeof(defaults[0]);

    for (int i = 0; i < count; ++i) {
        strncpy(out_games[i].tag, defaults[i].tag, sizeof(out_games[i].tag) - 1);
        strncpy(out_games[i].title, defaults[i].title, sizeof(out_games[i].title) - 1);
        strncpy(out_games[i].cmd, defaults[i].cmd, sizeof(out_games[i].cmd) - 1);
    }
    *out_count = count;
}
