#include <SDL/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "font.h"
#include "sound.h"
#include "boot.h"
#include "theme.h"
#include "hw.h"
#include "rom_scanner.h"

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define TARGET_FPS    60
#define FRAME_DELAY   (1000 / TARGET_FPS)
#define VISIBLE_ROWS  6

/* Dynamic Scanned Game List */
static ScannedGame g_games[MAX_SCANNED_GAMES];
static int g_game_count = 0;
static int g_selected_index = 0;
static int g_scroll_offset = 0;

/* UI State */
typedef enum {
    STATE_BOOT,
    STATE_MENU,
    STATE_SYS_MENU,
    STATE_SETTINGS
} AppState;

static AppState g_state = STATE_BOOT;

static int g_sys_menu_index = 0;
#define SYS_MENU_COUNT 4

static int g_settings_index = 0;
#define SETTINGS_COUNT 5

static SDL_Surface *g_screen = NULL;

/* Track modifier key states for simultaneous press (START + SELECT) */
static int g_key_start = 0;
static int g_key_select = 0;

/* Scan / Rescan ROMs */
static void scan_for_roms(void) {
    printf("[KURUI] Scanning for ROMs in /mnt/SDCARD/Roms/...\n");
    g_game_count = rom_scanner_scan("/mnt/SDCARD/Roms", g_games, MAX_SCANNED_GAMES);
    if (g_game_count <= 0) {
        printf("[KURUI] No ROMs found, loading curated fallback titles...\n");
        rom_scanner_load_defaults(g_games, &g_game_count);
    }
    printf("[KURUI] Total games available: %d\n", g_game_count);
    g_selected_index = 0;
    g_scroll_offset = 0;
}

/*
 * Initialize or Re-initialize SDL Subsystems
 */
static int init_subsystems(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    /* Double-buffered software surface */
    g_screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 0, SDL_SWSURFACE);
    if (!g_screen) {
        fprintf(stderr, "SDL_SetVideoMode failed: %s\n", SDL_GetError());
        return -1;
    }

#ifndef KURUI_VERSION
#define KURUI_VERSION "v0.0.1"
#endif

    SDL_WM_SetCaption("KURUI " KURUI_VERSION " // TRIMUI LAUNCHER", NULL);
    SDL_ShowCursor(SDL_DISABLE);

    if (sound_init() < 0) {
        fprintf(stderr, "sound_init warning (audio disabled): %s\n", SDL_GetError());
    }

    return 0;
}

/*
 * Full Subsystem Teardown
 */
static void shutdown_subsystems(void) {
    sound_close();
    SDL_Quit();
    g_screen = NULL;
}

/*
 * Launch Selected Game
 * Cleanly destroys SDL completely via SDL_Quit(), executes command, and re-inits on return.
 */
static void launch_game(int index) {
    if (index < 0 || index >= g_game_count) return;

    const ScannedGame *game = &g_games[index];
    printf("[KURUI] Shutting down SDL before launching: %s\n", game->title);

    /* 1. Full teardown of SDL */
    shutdown_subsystems();

    /* 2. Execute target command via system() */
    printf("[KURUI] Executing: %s\n", game->cmd);
    int ret = system(game->cmd);
    (void)ret;

    /* 3. Re-initialize SDL video & audio after return */
    printf("[KURUI] Re-initializing SDL after game exit...\n");
    if (init_subsystems() < 0) {
        fprintf(stderr, "[KURUI] Critical: Failed to re-initialize SDL!\n");
        exit(1);
    }
}

/*
 * Draw Industrial Menu Screen (320x240)
 */
static void draw_menu(SDL_Surface *surface) {
    const Theme *theme = theme_get();

    Uint32 col_bg        = SDL_MapRGB(surface->format, theme->bg_r, theme->bg_g, theme->bg_b);
    Uint32 col_header_bg = SDL_MapRGB(surface->format, theme->header_bg_r, theme->header_bg_g, theme->header_bg_b);
    Uint32 col_footer_bg = SDL_MapRGB(surface->format, theme->footer_bg_r, theme->footer_bg_g, theme->footer_bg_b);
    Uint32 col_border    = SDL_MapRGB(surface->format, theme->border_r, theme->border_g, theme->border_b);
    Uint32 col_accent    = SDL_MapRGB(surface->format, theme->accent_r, theme->accent_g, theme->accent_b);
    Uint32 col_text_pri  = SDL_MapRGB(surface->format, theme->text_primary_r, theme->text_primary_g, theme->text_primary_b);
    Uint32 col_text_dim  = SDL_MapRGB(surface->format, theme->text_dim_r, theme->text_dim_g, theme->text_dim_b);
    Uint32 col_text_tag  = SDL_MapRGB(surface->format, theme->text_tag_r, theme->text_tag_g, theme->text_tag_b);
    Uint32 col_text_inv  = SDL_MapRGB(surface->format, theme->text_inv_r, theme->text_inv_g, theme->text_inv_b);
    Uint32 col_highlight = SDL_MapRGB(surface->format, theme->highlight_r, theme->highlight_g, theme->highlight_b);

    /* 1. Full Surface Clear */
    SDL_FillRect(surface, NULL, col_bg);

    /* 2. Header (Height: 26px) */
    draw_fill_rect(surface, 0, 0, SCREEN_WIDTH, 26, col_header_bg);
    draw_line_h(surface, 0, 26, SCREEN_WIDTH, col_border);

    /* Header text (Bold KURUI in scale 2) */
    font_draw_string(surface, 8, 5, "KURUI", col_accent, 2);
    font_draw_string(surface, 96, 9, "// SELECT", col_text_pri, 1);

    /* Battery & Clock right aligned */
    char status_info[32];
    char time_str[16];
    hw_get_time_str(time_str, sizeof(time_str));
    int bat = hw_get_battery_percent();
    int charging = hw_is_charging();
    if (charging) {
        snprintf(status_info, sizeof(status_info), "[CHG %d%%] %s", bat, time_str);
    } else {
        snprintf(status_info, sizeof(status_info), "[%d%%] %s", bat, time_str);
    }

    int status_w = font_get_string_width(status_info, 1);
    font_draw_string(surface, SCREEN_WIDTH - status_w - 8, 9, status_info, col_text_dim, 1);

    /* 3. Main List (Height: 180px, Y: 27 to 205) */
    int list_start_y = 38;
    int row_height = 27;

    int end_row = g_scroll_offset + VISIBLE_ROWS;
    if (end_row > g_game_count) end_row = g_game_count;

    for (int i = g_scroll_offset; i < end_row; ++i) {
        int row_idx = i - g_scroll_offset;
        int item_y = list_start_y + row_idx * row_height;
        int is_selected = (i == g_selected_index);

        char idx_buf[8];
        snprintf(idx_buf, sizeof(idx_buf), "%02d", i + 1);

        if (is_selected) {
            /* Highlight bar */
            draw_fill_rect(surface, 6, item_y - 6, SCREEN_WIDTH - 20, 21, col_highlight);

            /* Selection Indicator */
            font_draw_string(surface, 12, item_y, ">", col_text_inv, 1);

            /* Index, Tag & Title in inverted dark text */
            font_draw_string(surface, 26, item_y, idx_buf, col_text_inv, 1);
            font_draw_string(surface, 52, item_y, g_games[i].tag, col_text_inv, 1);
            font_draw_string(surface, 108, item_y, g_games[i].title, col_text_inv, 1);
        } else {
            /* Normal item */
            font_draw_string(surface, 26, item_y, idx_buf, col_text_dim, 1);
            font_draw_string(surface, 52, item_y, g_games[i].tag, col_text_tag, 1);
            font_draw_string(surface, 108, item_y, g_games[i].title, col_text_pri, 1);
        }
    }

    /* 3.5 Teenage Engineering Slim Scroll Indicator */
    if (g_game_count > VISIBLE_ROWS) {
        int track_x = SCREEN_WIDTH - 8;
        int track_y = 34;
        int track_h = 166;
        draw_line_v(surface, track_x + 1, track_y, track_h, col_border);

        int thumb_h = (VISIBLE_ROWS * track_h) / g_game_count;
        if (thumb_h < 10) thumb_h = 10;
        int max_scroll = g_game_count - VISIBLE_ROWS;
        int thumb_y = track_y + (g_scroll_offset * (track_h - thumb_h)) / max_scroll;
        draw_fill_rect(surface, track_x, thumb_y, 3, thumb_h, col_accent);
    }

    /* 4. Footer (Height: 34px, Y: 206 to 240) */
    draw_line_h(surface, 0, 206, SCREEN_WIDTH, col_border);
    draw_fill_rect(surface, 0, 207, SCREEN_WIDTH, 33, col_footer_bg);

    /* Action guides */
    font_draw_string(surface, 14, 218, "A: LAUNCH", col_text_pri, 1);

    char page_info[32];
    snprintf(page_info, sizeof(page_info), "%d/%d", g_selected_index + 1, g_game_count);
    font_draw_string(surface, 136, 218, page_info, col_text_dim, 1);

    font_draw_string(surface, 222, 218, "ST+SEL: EXIT", col_text_dim, 1);
}

/*
 * Draw System Menu Modal (Centered dialog on top of main menu)
 */
static void draw_sys_menu(SDL_Surface *surface) {
    draw_menu(surface);

    const Theme *theme = theme_get();

    int card_w = 230;
    int card_h = 136;
    int card_x = (SCREEN_WIDTH - card_w) / 2;
    int card_y = (SCREEN_HEIGHT - card_h) / 2;

    Uint32 col_card_bg   = SDL_MapRGB(surface->format, theme->card_bg_r, theme->card_bg_g, theme->card_bg_b);
    Uint32 col_card_head = SDL_MapRGB(surface->format, theme->card_header_r, theme->card_header_g, theme->card_header_b);
    Uint32 col_border    = SDL_MapRGB(surface->format, theme->accent_r, theme->accent_g, theme->accent_b);
    Uint32 col_div       = SDL_MapRGB(surface->format, theme->border_r, theme->border_g, theme->border_b);
    Uint32 col_accent    = SDL_MapRGB(surface->format, theme->accent_r, theme->accent_g, theme->accent_b);
    Uint32 col_text_pri  = SDL_MapRGB(surface->format, theme->text_primary_r, theme->text_primary_g, theme->text_primary_b);
    Uint32 col_text_dim  = SDL_MapRGB(surface->format, theme->text_dim_r, theme->text_dim_g, theme->text_dim_b);
    Uint32 col_text_inv  = SDL_MapRGB(surface->format, theme->text_inv_r, theme->text_inv_g, theme->text_inv_b);
    Uint32 col_highlight = SDL_MapRGB(surface->format, theme->highlight_r, theme->highlight_g, theme->highlight_b);

    /* Card background & Orange border */
    draw_fill_rect(surface, card_x, card_y, card_w, card_h, col_card_bg);
    draw_rect(surface, card_x, card_y, card_w, card_h, col_border);

    /* Modal header (Height: 24px) */
    draw_fill_rect(surface, card_x + 1, card_y + 1, card_w - 2, 23, col_card_head);
    draw_line_h(surface, card_x, card_y + 24, card_w, col_div);
    font_draw_string(surface, card_x + 10, card_y + 8, "// SYSTEM MENU", col_accent, 1);

    /* Options */
    static const char *options[SYS_MENU_COUNT] = {
        "RESUME",
        "RETURN TO TRIMUI UI",
        "SETTINGS",
        "POWER OFF"
    };

    int item_start_y = card_y + 34;
    int row_h = 19;

    for (int i = 0; i < SYS_MENU_COUNT; ++i) {
        int item_y = item_start_y + i * row_h;
        int is_selected = (i == g_sys_menu_index);

        if (is_selected) {
            draw_fill_rect(surface, card_x + 6, item_y - 2, card_w - 12, 16, col_highlight);
            font_draw_string(surface, card_x + 12, item_y + 2, ">", col_text_inv, 1);
            font_draw_string(surface, card_x + 24, item_y + 2, options[i], col_text_inv, 1);
        } else {
            font_draw_string(surface, card_x + 24, item_y + 2, options[i], col_text_pri, 1);
        }
    }

    /* Modal footer */
    draw_line_h(surface, card_x, card_y + card_h - 22, card_w, col_div);
    font_draw_string(surface, card_x + 14, card_y + card_h - 14, "A: SELECT    B/MENU: BACK", col_text_dim, 1);
}

/*
 * Draw Settings Menu Modal (Centered dialog on top of main menu)
 */
static void draw_settings_menu(SDL_Surface *surface) {
    draw_menu(surface);

    const Theme *theme = theme_get();

    int card_w = 260;
    int card_h = 160;
    int card_x = (SCREEN_WIDTH - card_w) / 2;
    int card_y = (SCREEN_HEIGHT - card_h) / 2;

    Uint32 col_card_bg   = SDL_MapRGB(surface->format, theme->card_bg_r, theme->card_bg_g, theme->card_bg_b);
    Uint32 col_card_head = SDL_MapRGB(surface->format, theme->card_header_r, theme->card_header_g, theme->card_header_b);
    Uint32 col_border    = SDL_MapRGB(surface->format, theme->accent_r, theme->accent_g, theme->accent_b);
    Uint32 col_div       = SDL_MapRGB(surface->format, theme->border_r, theme->border_g, theme->border_b);
    Uint32 col_accent    = SDL_MapRGB(surface->format, theme->accent_r, theme->accent_g, theme->accent_b);
    Uint32 col_text_pri  = SDL_MapRGB(surface->format, theme->text_primary_r, theme->text_primary_g, theme->text_primary_b);
    Uint32 col_text_dim  = SDL_MapRGB(surface->format, theme->text_dim_r, theme->text_dim_g, theme->text_dim_b);
    Uint32 col_text_inv  = SDL_MapRGB(surface->format, theme->text_inv_r, theme->text_inv_g, theme->text_inv_b);
    Uint32 col_highlight = SDL_MapRGB(surface->format, theme->highlight_r, theme->highlight_g, theme->highlight_b);

    /* Card background & Orange border */
    draw_fill_rect(surface, card_x, card_y, card_w, card_h, col_card_bg);
    draw_rect(surface, card_x, card_y, card_w, card_h, col_border);

    /* Modal header (Height: 24px) */
    draw_fill_rect(surface, card_x + 1, card_y + 1, card_w - 2, 23, col_card_head);
    draw_line_h(surface, card_x, card_y + 24, card_w, col_div);
    font_draw_string(surface, card_x + 10, card_y + 8, "// SETTINGS", col_accent, 1);

    char theme_opt[36];
    snprintf(theme_opt, sizeof(theme_opt), "THEME       : [%s]", theme_get_name());

    char sound_opt[36];
    snprintf(sound_opt, sizeof(sound_opt), "SOUND FX    : [%s]", sound_is_enabled() ? "ENABLED" : "MUTED");

    char count_opt[36];
    snprintf(count_opt, sizeof(count_opt), "ROMS LOADED : [%d TITLES]", g_game_count);

    const char *settings_opts[SETTINGS_COUNT] = {
        theme_opt,
        sound_opt,
        count_opt,
        "RE-SCAN ROMS",
        "< BACK TO SYSTEM MENU"
    };

    int item_start_y = card_y + 34;
    int row_h = 19;

    for (int i = 0; i < SETTINGS_COUNT; ++i) {
        int item_y = item_start_y + i * row_h;
        int is_selected = (i == g_settings_index);

        if (is_selected) {
            draw_fill_rect(surface, card_x + 6, item_y - 2, card_w - 12, 16, col_highlight);
            font_draw_string(surface, card_x + 10, item_y + 2, ">", col_text_inv, 1);
            font_draw_string(surface, card_x + 22, item_y + 2, settings_opts[i], col_text_inv, 1);
        } else {
            font_draw_string(surface, card_x + 22, item_y + 2, settings_opts[i], col_text_pri, 1);
        }
    }

    /* Modal footer */
    draw_line_h(surface, card_x, card_y + card_h - 22, card_w, col_div);
    font_draw_string(surface, card_x + 14, card_y + card_h - 14, "A: CHANGE/OK    B: BACK", col_text_dim, 1);
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    theme_init();

    if (init_subsystems() < 0) {
        return 1;
    }

    /* Scan ROMs */
    scan_for_roms();

    boot_init();
    g_state = STATE_BOOT;

    int running = 1;
    SDL_Event event;

    while (running) {
        Uint32 frame_start = SDL_GetTicks();

        /* Input Processing */
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_KEYDOWN) {
                SDLKey key = event.key.keysym.sym;

                /* Track modifier keys for simultaneous press check */
                if (key == SDLK_RETURN) g_key_start = 1;
                if (key == SDLK_RCTRL || key == SDLK_SPACE || key == SDLK_RSHIFT) g_key_select = 1;

                /* START + SELECT simultaneous press -> Exit safely to MainUI */
                if (g_key_start && g_key_select) {
                    printf("[KURUI] START + SELECT detected. Exiting safely to stock MainUI...\n");
                    running = 0;
                    break;
                }

                if (g_state == STATE_MENU) {
                    /* Up / Down Navigation */
                    if (key == SDLK_UP) {
                        if (g_selected_index > 0) {
                            g_selected_index--;
                        } else {
                            g_selected_index = g_game_count - 1;
                        }
                        /* Adjust scroll offset */
                        if (g_selected_index < g_scroll_offset) {
                            g_scroll_offset = g_selected_index;
                        } else if (g_selected_index >= g_scroll_offset + VISIBLE_ROWS) {
                            g_scroll_offset = g_selected_index - VISIBLE_ROWS + 1;
                        }
                    } else if (key == SDLK_DOWN) {
                        if (g_selected_index < g_game_count - 1) {
                            g_selected_index++;
                        } else {
                            g_selected_index = 0;
                        }
                        /* Adjust scroll offset */
                        if (g_selected_index < g_scroll_offset) {
                            g_scroll_offset = g_selected_index;
                        } else if (g_selected_index >= g_scroll_offset + VISIBLE_ROWS) {
                            g_scroll_offset = g_selected_index - VISIBLE_ROWS + 1;
                        }
                    }
                    /* A Button (Confirm / Launch): LCTRL on TRIMUI, 'z' or Return on PC */
                    else if (key == SDLK_LCTRL || key == SDLK_z || (!g_key_select && key == SDLK_RETURN)) {
                        launch_game(g_selected_index);
                    }
                    /* MENU Button (Open System Menu): ESCAPE on TRIMUI and PC */
                    else if (key == SDLK_ESCAPE) {
                        printf("[KURUI] MENU button pressed. Opening System Menu...\n");
                        g_state = STATE_SYS_MENU;
                        g_sys_menu_index = 0;
                    }
                    /* B Button (Cancel / No-op) */
                    else if (key == SDLK_LALT || key == SDLK_x) {
                        printf("[KURUI] B Button pressed.\n");
                    }
                } else if (g_state == STATE_SYS_MENU) {
                    /* Up / Down Navigation in System Menu */
                    if (key == SDLK_UP) {
                        g_sys_menu_index = (g_sys_menu_index > 0) ? (g_sys_menu_index - 1) : (SYS_MENU_COUNT - 1);
                    } else if (key == SDLK_DOWN) {
                        g_sys_menu_index = (g_sys_menu_index < SYS_MENU_COUNT - 1) ? (g_sys_menu_index + 1) : 0;
                    }
                    /* A Button (Confirm Selection) */
                    else if (key == SDLK_LCTRL || key == SDLK_z || (!g_key_select && key == SDLK_RETURN)) {
                        if (g_sys_menu_index == 0) {
                            /* RESUME */
                            g_state = STATE_MENU;
                        } else if (g_sys_menu_index == 1) {
                            /* RETURN TO TRIMUI UI */
                            printf("[KURUI] Returning cleanly to stock MainUI...\n");
                            running = 0;
                            break;
                        } else if (g_sys_menu_index == 2) {
                            /* SETTINGS */
                            g_state = STATE_SETTINGS;
                            g_settings_index = 0;
                        } else if (g_sys_menu_index == 3) {
                            /* POWER OFF */
                            printf("[KURUI] Power off requested from System Menu...\n");
                            shutdown_subsystems();
                            system("poweroff");
                            exit(0);
                        }
                    }
                    /* B Button or MENU Button (Cancel / Return to Game List) */
                    else if (key == SDLK_ESCAPE || key == SDLK_LALT || key == SDLK_x) {
                        g_state = STATE_MENU;
                    }
                } else if (g_state == STATE_SETTINGS) {
                    /* Up / Down Navigation in Settings */
                    if (key == SDLK_UP) {
                        g_settings_index = (g_settings_index > 0) ? (g_settings_index - 1) : (SETTINGS_COUNT - 1);
                    } else if (key == SDLK_DOWN) {
                        g_settings_index = (g_settings_index < SETTINGS_COUNT - 1) ? (g_settings_index + 1) : 0;
                    }
                    /* A Button */
                    else if (key == SDLK_LCTRL || key == SDLK_z || (!g_key_select && key == SDLK_RETURN)) {
                        if (g_settings_index == 0) {
                            /* Cycle theme */
                            theme_cycle_next();
                            sound_trigger_pikoon();
                        } else if (g_settings_index == 1) {
                            /* Toggle sound */
                            sound_set_enabled(!sound_is_enabled());
                            if (sound_is_enabled()) {
                                sound_trigger_pikoon();
                            }
                        } else if (g_settings_index == 3) {
                            /* Re-scan ROMs */
                            scan_for_roms();
                            sound_trigger_pikoon();
                        } else if (g_settings_index == 4) {
                            /* BACK */
                            g_state = STATE_SYS_MENU;
                        }
                    }
                    /* B Button or MENU Button (Return to System Menu) */
                    else if (key == SDLK_ESCAPE || key == SDLK_LALT || key == SDLK_x) {
                        g_state = STATE_SYS_MENU;
                    }
                } else if (g_state == STATE_BOOT) {
                    /* Any key press skips boot animation directly to menu */
                    if (key == SDLK_RETURN || key == SDLK_SPACE || key == SDLK_LCTRL || key == SDLK_z || key == SDLK_ESCAPE) {
                        g_state = STATE_MENU;
                    }
                }
            } else if (event.type == SDL_KEYUP) {
                SDLKey key = event.key.keysym.sym;
                if (key == SDLK_RETURN) g_key_start = 0;
                if (key == SDLK_RCTRL || key == SDLK_SPACE || key == SDLK_RSHIFT) g_key_select = 0;
            }
        }

        if (!running) break;

        /* State Update & Drawing */
        if (g_state == STATE_BOOT) {
            if (boot_update()) {
                g_state = STATE_MENU;
            }
            boot_draw(g_screen);
        } else if (g_state == STATE_MENU) {
            draw_menu(g_screen);
        } else if (g_state == STATE_SYS_MENU) {
            draw_sys_menu(g_screen);
        } else if (g_state == STATE_SETTINGS) {
            draw_settings_menu(g_screen);
        }

        /* Buffer Flip */
        SDL_Flip(g_screen);

        /* Strict Frame-rate control with SDL_Delay to prevent 100% CPU lock */
        Uint32 frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < FRAME_DELAY) {
            SDL_Delay(FRAME_DELAY - frame_time);
        }
    }

    shutdown_subsystems();
    return 0;
}
