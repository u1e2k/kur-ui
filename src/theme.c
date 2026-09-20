#include "theme.h"

static ThemeType g_current_theme = THEME_MATTE_DARK;

static const Theme g_themes[THEME_COUNT] = {
    /* 0: MATTE DARK (Signature Teenage Engineering Charcoal + Orange) */
    {
        .type = THEME_MATTE_DARK,
        .name = "MATTE DARK",
        .bg_r = 20, .bg_g = 20, .bg_b = 23,
        .header_bg_r = 28, .header_bg_g = 28, .header_bg_b = 33,
        .footer_bg_r = 24, .footer_bg_g = 24, .footer_bg_b = 28,
        .border_r = 50, .border_g = 50, .border_b = 58,
        .accent_r = 255, .accent_g = 85, .accent_b = 0,
        .text_primary_r = 245, .text_primary_g = 245, .text_primary_b = 245,
        .text_dim_r = 120, .text_dim_g = 120, .text_dim_b = 130,
        .text_tag_r = 160, .text_tag_g = 160, .text_tag_b = 170,
        .text_inv_r = 15, .text_inv_g = 15, .text_inv_b = 15,
        .highlight_r = 255, .highlight_g = 85, .highlight_b = 0,
        .card_bg_r = 16, .card_bg_g = 16, .card_bg_b = 20,
        .card_header_r = 28, .card_header_g = 28, .card_header_b = 34
    },
    /* 1: OFF-WHITE (OP-1 / Braun Industrial Aesthetic) */
    {
        .type = THEME_OFF_WHITE,
        .name = "OFF-WHITE",
        .bg_r = 226, .bg_g = 226, .bg_b = 230,
        .header_bg_r = 212, .header_bg_g = 212, .header_bg_b = 216,
        .footer_bg_r = 208, .footer_bg_g = 208, .footer_bg_b = 212,
        .border_r = 184, .border_g = 184, .border_b = 192,
        .accent_r = 255, .accent_g = 85, .accent_b = 0,
        .text_primary_r = 24, .text_primary_g = 24, .text_primary_b = 28,
        .text_dim_r = 110, .text_dim_g = 110, .text_dim_b = 120,
        .text_tag_r = 45, .text_tag_g = 75, .text_tag_b = 160, /* Cobalt Blue */
        .text_inv_r = 245, .text_inv_g = 245, .text_inv_b = 245,
        .highlight_r = 30, .highlight_g = 30, .highlight_b = 35, /* Dark bold selection */
        .card_bg_r = 238, .card_bg_g = 238, .card_bg_b = 242,
        .card_header_r = 218, .card_header_g = 218, .card_header_b = 224
    },
    /* 2: DMG GREEN (Classic Game Boy Monochrome) */
    {
        .type = THEME_DMG_GREEN,
        .name = "DMG GREEN",
        .bg_r = 139, .bg_g = 149, .bg_b = 109,
        .header_bg_r = 155, .header_bg_g = 168, .header_bg_b = 115,
        .footer_bg_r = 128, .footer_bg_g = 138, .footer_bg_b = 98,
        .border_r = 74, .border_g = 84, .border_b = 50,
        .accent_r = 31, .accent_g = 36, .accent_b = 10,
        .text_primary_r = 15, .text_primary_g = 40, .text_primary_b = 12,
        .text_dim_r = 60, .text_dim_g = 75, .text_dim_b = 35,
        .text_tag_r = 40, .text_tag_g = 55, .text_tag_b = 20,
        .text_inv_r = 155, .text_inv_g = 188, .text_inv_b = 15,
        .highlight_r = 25, .highlight_g = 35, .highlight_b = 12,
        .card_bg_r = 139, .card_bg_g = 149, .card_bg_b = 109,
        .card_header_r = 120, .card_header_g = 130, .card_header_b = 90
    }
};

void theme_init(void) {
    g_current_theme = THEME_MATTE_DARK;
}

ThemeType theme_get_type(void) {
    return g_current_theme;
}

void theme_set_type(ThemeType type) {
    if (type >= 0 && type < THEME_COUNT) {
        g_current_theme = type;
    }
}

void theme_cycle_next(void) {
    g_current_theme = (g_current_theme + 1) % THEME_COUNT;
}

const Theme *theme_get(void) {
    return &g_themes[g_current_theme];
}

const char *theme_get_name(void) {
    return g_themes[g_current_theme].name;
}
