#include "boot.h"
#include "font.h"
#include "sound.h"

#ifndef KURUI_VERSION
#define KURUI_VERSION "v0.0.1"
#endif

#define TARGET_Y 96
#define LOGO_SCALE 3
#define REST_FRAMES 60 /* ~1.0 second hold after landing at 60 FPS */

static int g_logo_y = -32;
static int g_landed = 0;
static int g_hold_timer = 0;
static int g_finished = 0;

void boot_init(void) {
    g_logo_y = -32;
    g_landed = 0;
    g_hold_timer = 0;
    g_finished = 0;
}

int boot_update(void) {
    if (g_finished) return 1;

    if (!g_landed) {
        /* Smooth, elegant Game Boy descent tempo (~2.1 seconds) */
        g_logo_y += 1;
        if (g_logo_y >= TARGET_Y) {
            g_logo_y = TARGET_Y;
            g_landed = 1;
            /* Instantaneous audio trigger right when landing */
            sound_trigger_pikoon();
        }
    } else {
        g_hold_timer++;
        if (g_hold_timer >= REST_FRAMES) {
            g_finished = 1;
            return 1;
        }
    }

    return 0;
}

void boot_draw(SDL_Surface *surface) {
    if (!surface) return;

    /* Industrial Matte Charcoal Background */
    Uint32 bg_color = SDL_MapRGB(surface->format, 20, 20, 22);
    draw_fill_rect(surface, 0, 0, surface->w, surface->h, bg_color);

    /* "KURUI" Logo */
    const char *logo_text = "KURUI";
    int text_width = font_get_string_width(logo_text, LOGO_SCALE);
    int logo_x = (surface->w - text_width) / 2;

    Uint32 logo_color = SDL_MapRGB(surface->format, 240, 240, 240);
    Uint32 accent_orange = SDL_MapRGB(surface->format, 255, 85, 0);

    /* Draw Logo */
    font_draw_string(surface, logo_x, g_logo_y, logo_text, logo_color, LOGO_SCALE);

    /* Registered Symbol or Accent Dot on landing */
    if (g_landed) {
        draw_fill_rect(surface, logo_x + text_width + 4, g_logo_y + 2, 4, 4, accent_orange);

        /* Subtitle appearing right after landing with dynamic version */
        Uint32 dim_color = SDL_MapRGB(surface->format, 120, 120, 125);
        const char *sub = "TRIMUI INDUSTRIAL " KURUI_VERSION;
        int sub_w = font_get_string_width(sub, 1);
        int sub_x = (surface->w - sub_w) / 2;
        font_draw_string(surface, sub_x, TARGET_Y + 36, sub, dim_color, 1);
    }
}
