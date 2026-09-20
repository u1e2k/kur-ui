#include <SDL/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "font.h"
#include "sound.h"
#include "boot.h"

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define TARGET_FPS    60
#define FRAME_DELAY   (1000 / TARGET_FPS)

/* Static Game List Entry */
typedef struct {
    const char *tag;        /* Console tag, e.g., "[GB]" */
    const char *title;      /* Title in UTF-8 */
    const char *cmd_trimui; /* Actual launch script on TRIMUI */
    const char *cmd_pc;     /* Fallback dummy command for PC */
} GameEntry;

/* Curated 6 games for quick access */
static const GameEntry g_games[] = {
    {"[GB]",   "テトリス",           "/mnt/SDCARD/Emus/gb/launch.sh \"/mnt/SDCARD/Roms/gb/tetris.gb\"", "echo [LAUNCH] GB Tetris"},
    {"[FC]",   "スーパーマリオ",     "/mnt/SDCARD/Emus/fc/launch.sh \"/mnt/SDCARD/Roms/fc/mario.nes\"", "echo [LAUNCH] FC Super Mario"},
    {"[GBA]",  "ゼルダの伝説",       "/mnt/SDCARD/Emus/gba/launch.sh \"/mnt/SDCARD/Roms/gba/zelda.gba\"", "echo [LAUNCH] GBA Zelda"},
    {"[GB]",   "ポケットモンスター", "/mnt/SDCARD/Emus/gb/launch.sh \"/mnt/SDCARD/Roms/gb/pokemon.gb\"", "echo [LAUNCH] GB Pokemon"},
    {"[NGPC]", "メタルスラッグ",     "/mnt/SDCARD/Emus/ngpc/launch.sh \"/mnt/SDCARD/Roms/ngpc/mslug.ngc\"", "echo [LAUNCH] NGPC Metal Slug"},
    {"[FC]",   "魔界村",             "/mnt/SDCARD/Emus/fc/launch.sh \"/mnt/SDCARD/Roms/fc/makaimura.nes\"", "echo [LAUNCH] FC Makaimura"}
};
#define GAME_COUNT (sizeof(g_games) / sizeof(g_games[0]))

/* UI State */
typedef enum {
    STATE_BOOT,
    STATE_MENU
} AppState;

static AppState g_state = STATE_BOOT;
static int g_selected_index = 0;
static SDL_Surface *g_screen = NULL;

/* Track modifier key states for simultaneous press (START + SELECT) */
static int g_key_start = 0;
static int g_key_select = 0;

/* Check if running on TRIMUI hardware */
static int is_trimui_hardware(void) {
    FILE *f = fopen("/mnt/SDCARD", "r");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}

/*
 * Initialize or Re-initialize SDL Subsystems
 */
static int init_subsystems(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    /* TRIMUI uses 16-bit framebuffer (fb0). PC default is typically 32-bit. */
    /* SDL_SWSURFACE with double buffering */
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
    if (index < 0 || index >= (int)GAME_COUNT) return;

    const GameEntry *game = &g_games[index];
    int on_trimui = is_trimui_hardware();
    const char *cmd = on_trimui ? game->cmd_trimui : game->cmd_pc;

    printf("[KURUI] Completely shutting down SDL before launching: %s\n", game->title);

    /* 1. Full teardown of SDL */
    shutdown_subsystems();

    /* 2. Execute target command via system() */
    printf("[KURUI] Executing: %s\n", cmd);
    int ret = system(cmd);
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
    /* Palette definition mapped through surface format */
    Uint32 col_bg        = SDL_MapRGB(surface->format, 20, 20, 23);       /* Matte Dark */
    Uint32 col_header_bg = SDL_MapRGB(surface->format, 28, 28, 33);       /* Header Dark */
    Uint32 col_footer_bg = SDL_MapRGB(surface->format, 24, 24, 28);       /* Footer Dark */
    Uint32 col_border    = SDL_MapRGB(surface->format, 50, 50, 58);       /* Divider */
    Uint32 col_orange    = SDL_MapRGB(surface->format, 255, 85, 0);       /* Accent TE Orange */
    Uint32 col_white     = SDL_MapRGB(surface->format, 245, 245, 245);    /* Pure White */
    Uint32 col_tag       = SDL_MapRGB(surface->format, 160, 160, 170);    /* Muted Tag */
    Uint32 col_dim       = SDL_MapRGB(surface->format, 120, 120, 130);    /* Dim text */
    Uint32 col_black     = SDL_MapRGB(surface->format, 15, 15, 15);       /* Inverted text */

    /* 1. Background */
    draw_fill_rect(surface, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, col_bg);

    /* 2. Header (Height: 24px) */
    draw_fill_rect(surface, 0, 0, SCREEN_WIDTH, 24, col_header_bg);
    draw_line_h(surface, 0, 24, SCREEN_WIDTH, col_border);

    /* Header text */
    font_draw_string(surface, 8, 8, "KURUI", col_orange, 1);
    font_draw_string(surface, 52, 8, "// SELECT", col_white, 1);

    /* Battery & Clock right aligned */
    const char *status_info = "[85%] 12:00";
    int status_w = font_get_string_width(status_info, 1);
    font_draw_string(surface, SCREEN_WIDTH - status_w - 8, 8, status_info, col_dim, 1);

    /* 3. Main List (Height: 184px, Y: 25 to 208) */
    int list_start_y = 30;
    int row_height = 28;

    for (int i = 0; i < (int)GAME_COUNT; ++i) {
        int item_y = list_start_y + i * row_height;
        int is_selected = (i == g_selected_index);

        if (is_selected) {
            /* Highlight bar */
            draw_fill_rect(surface, 6, item_y - 2, SCREEN_WIDTH - 12, 22, col_orange);

            /* Arrow indicator */
            font_draw_string(surface, 12, item_y + 4, ">", col_black, 1);

            /* Tag & Title in inverted dark text */
            font_draw_string(surface, 26, item_y + 4, g_games[i].tag, col_black, 1);
            int tag_w = font_get_string_width(g_games[i].tag, 1);
            font_draw_string(surface, 32 + tag_w, item_y + 4, g_games[i].title, col_black, 1);
        } else {
            /* Normal item */
            font_draw_string(surface, 26, item_y + 4, g_games[i].tag, col_tag, 1);
            int tag_w = font_get_string_width(g_games[i].tag, 1);
            font_draw_string(surface, 32 + tag_w, item_y + 4, g_games[i].title, col_white, 1);
        }
    }

    /* 4. Footer (Height: 32px, Y: 208 to 240) */
    draw_line_h(surface, 0, 208, SCREEN_WIDTH, col_border);
    draw_fill_rect(surface, 0, 209, SCREEN_WIDTH, 31, col_footer_bg);

    /* Action guides */
    font_draw_string(surface, 10, 218, "A: 起動", col_white, 1);
    font_draw_string(surface, 90, 218, "B: 電源OFF", col_dim, 1);
    font_draw_string(surface, 200, 218, "ST+SEL: 終了", col_dim, 1);
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    if (init_subsystems() < 0) {
        return 1;
    }

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

                /* START + SELECT simultaneous press -> Exit */
                if (g_key_start && g_key_select) {
                    printf("[KURUI] START + SELECT detected. Exiting safely...\n");
                    running = 0;
                    break;
                }

                if (g_state == STATE_MENU) {
                    /* Up / Down Navigation */
                    if (key == SDLK_UP) {
                        if (g_selected_index > 0) {
                            g_selected_index--;
                        } else {
                            g_selected_index = (int)GAME_COUNT - 1;
                        }
                    } else if (key == SDLK_DOWN) {
                        if (g_selected_index < (int)GAME_COUNT - 1) {
                            g_selected_index++;
                        } else {
                            g_selected_index = 0;
                        }
                    }
                    /* A Button (Confirm / Launch): LCTRL on TRIMUI, 'z' or Return on PC */
                    else if (key == SDLK_LCTRL || key == SDLK_z || (!g_key_select && key == SDLK_RETURN)) {
                        launch_game(g_selected_index);
                    }
                    /* B Button (Power Off / Exit): LALT on TRIMUI, 'x' or Escape on PC */
                    else if (key == SDLK_LALT || key == SDLK_x || key == SDLK_ESCAPE) {
                        printf("[KURUI] B Button pressed. Shutting down...\n");
                        if (is_trimui_hardware()) {
                            system("poweroff");
                        }
                        running = 0;
                        break;
                    }
                } else if (g_state == STATE_BOOT) {
                    /* Any key press skips boot animation directly to menu */
                    if (key == SDLK_RETURN || key == SDLK_SPACE || key == SDLK_LCTRL || key == SDLK_z) {
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
