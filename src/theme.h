#ifndef THEME_H
#define THEME_H

#include <SDL/SDL.h>

typedef enum {
    THEME_MATTE_DARK = 0,
    THEME_OFF_WHITE,
    THEME_DMG_GREEN,
    THEME_COUNT
} ThemeType;

typedef struct {
    ThemeType type;
    const char *name;
    /* RGB definitions */
    Uint8 bg_r, bg_g, bg_b;
    Uint8 header_bg_r, header_bg_g, header_bg_b;
    Uint8 footer_bg_r, footer_bg_g, footer_bg_b;
    Uint8 border_r, border_g, border_b;
    Uint8 accent_r, accent_g, accent_b;
    Uint8 text_primary_r, text_primary_g, text_primary_b;
    Uint8 text_dim_r, text_dim_g, text_dim_b;
    Uint8 text_tag_r, text_tag_g, text_tag_b;
    Uint8 text_inv_r, text_inv_g, text_inv_b;
    Uint8 highlight_r, highlight_g, highlight_b;
    Uint8 card_bg_r, card_bg_g, card_bg_b;
    Uint8 card_header_r, card_header_g, card_header_b;
} Theme;

/* Theme Lifecycle & Navigation */
void theme_init(void);
ThemeType theme_get_type(void);
void theme_set_type(ThemeType type);
void theme_cycle_next(void);
const Theme *theme_get(void);
const char *theme_get_name(void);

#endif /* THEME_H */
