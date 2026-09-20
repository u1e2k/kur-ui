#ifndef ROM_SCANNER_H
#define ROM_SCANNER_H

#define MAX_SCANNED_GAMES 256

typedef struct {
    char tag[12];     /* System tag, e.g. "[GB]" */
    char title[64];   /* Cleaned uppercase title */
    char cmd[512];    /* Launch command */
} ScannedGame;

/* Scan /mnt/SDCARD/Roms/ (or custom path) and populate out_games. Returns count of games found. */
int rom_scanner_scan(const char *roms_dir, ScannedGame *out_games, int max_games);

/* Fallback curated list when no ROMs found or running on PC */
void rom_scanner_load_defaults(ScannedGame *out_games, int *out_count);

#endif /* ROM_SCANNER_H */
