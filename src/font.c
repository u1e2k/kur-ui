#include "font.h"
#include "font_data.h"

/*
 * Direct Pixel Drawing Routine
 * Safely handles 16-bit (RGB565 on TRIMUI) and 32-bit (ARGB on PC) surfaces.
 */
void draw_pixel(SDL_Surface *surface, int x, int y, Uint32 color) {
    if (!surface || x < 0 || y < 0 || x >= surface->w || y >= surface->h) {
        return;
    }

    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch;
    int bpp = surface->format->BytesPerPixel;

    if (bpp == 2) {
        *(Uint16 *)(p + x * 2) = (Uint16)color;
    } else if (bpp == 4) {
        *(Uint32 *)(p + x * 4) = (Uint32)color;
    }
}

/*
 * Horizontal Line
 */
void draw_line_h(SDL_Surface *surface, int x, int y, int w, Uint32 color) {
    if (!surface || y < 0 || y >= surface->h || w <= 0) return;
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (x + w > surface->w) {
        w = surface->w - x;
    }
    if (w <= 0) return;

    int bpp = surface->format->BytesPerPixel;
    Uint8 *row = (Uint8 *)surface->pixels + y * surface->pitch;

    if (bpp == 2) {
        Uint16 *dst = (Uint16 *)(row + x * 2);
        Uint16 val = (Uint16)color;
        for (int i = 0; i < w; ++i) {
            dst[i] = val;
        }
    } else if (bpp == 4) {
        Uint32 *dst = (Uint32 *)(row + x * 4);
        for (int i = 0; i < w; ++i) {
            dst[i] = color;
        }
    }
}

/*
 * Vertical Line
 */
void draw_line_v(SDL_Surface *surface, int x, int y, int h, Uint32 color) {
    if (!surface || x < 0 || x >= surface->w || h <= 0) return;
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (y + h > surface->h) {
        h = surface->h - y;
    }
    if (h <= 0) return;

    int bpp = surface->format->BytesPerPixel;
    int pitch = surface->pitch;
    Uint8 *row = (Uint8 *)surface->pixels + y * pitch + x * bpp;

    if (bpp == 2) {
        Uint16 val = (Uint16)color;
        for (int i = 0; i < h; ++i) {
            *(Uint16 *)row = val;
            row += pitch;
        }
    } else if (bpp == 4) {
        for (int i = 0; i < h; ++i) {
            *(Uint32 *)row = color;
            row += pitch;
        }
    }
}

/*
 * Filled Rectangle
 */
void draw_fill_rect(SDL_Surface *surface, int x, int y, int w, int h, Uint32 color) {
    if (!surface || w <= 0 || h <= 0) return;
    SDL_Rect rect;
    rect.x = (Sint16)x;
    rect.y = (Sint16)y;
    rect.w = (Uint16)w;
    rect.h = (Uint16)h;
    SDL_FillRect(surface, &rect, color);
}

/*
 * Outlined Rectangle
 */
void draw_rect(SDL_Surface *surface, int x, int y, int w, int h, Uint32 color) {
    if (w <= 0 || h <= 0) return;
    draw_line_h(surface, x, y, w, color);
    draw_line_h(surface, x, y + h - 1, w, color);
    draw_line_v(surface, x, y, h, color);
    draw_line_v(surface, x + w - 1, y, h, color);
}

/*
 * Binary Search for Japanese Glyphs
 */
static const uint8_t *lookup_japanese_glyph(uint32_t codepoint) {
    int low = 0;
    int high = (int)FONT_JP_COUNT - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (font_jp_table[mid].codepoint == codepoint) {
            return font_jp_table[mid].data;
        }
        if (font_jp_table[mid].codepoint < codepoint) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return NULL;
}

/*
 * Fallback pattern for missing glyphs (box with dot in middle)
 */
static const uint8_t glyph_fallback[8] = {
    0x7e, 0x42, 0x42, 0x5a, 0x42, 0x42, 0x7e, 0x00
};

/*
 * Draw single character by Unicode codepoint
 */
void font_draw_char(SDL_Surface *surface, int x, int y, uint32_t codepoint, Uint32 color, int scale) {
    const uint8_t *bitmap = NULL;

    if (codepoint >= 0x20 && codepoint <= 0x7E) {
        bitmap = font_ascii[codepoint - 0x20];
    } else {
        bitmap = lookup_japanese_glyph(codepoint);
        if (!bitmap) {
            bitmap = glyph_fallback;
        }
    }

    if (scale <= 1) {
        for (int row = 0; row < 8; ++row) {
            int py = y + row;
            if (py < 0 || py >= surface->h) continue;
            uint8_t bits = bitmap[row];
            for (int col = 0; col < 8; ++col) {
                if (bits & (0x80 >> col)) {
                    draw_pixel(surface, x + col, py, color);
                }
            }
        }
    } else {
        for (int row = 0; row < 8; ++row) {
            uint8_t bits = bitmap[row];
            for (int col = 0; col < 8; ++col) {
                if (bits & (0x80 >> col)) {
                    draw_fill_rect(surface, x + col * scale, y + row * scale, scale, scale, color);
                }
            }
        }
    }
}

/*
 * Decode UTF-8 sequence to Unicode Codepoint
 * Returns number of bytes consumed.
 */
static int decode_utf8(const char *str, uint32_t *codepoint) {
    const uint8_t *s = (const uint8_t *)str;
    if (s[0] == '\0') {
        *codepoint = 0;
        return 0;
    }

    if (s[0] < 0x80) {
        *codepoint = s[0];
        return 1;
    } else if ((s[0] & 0xE0) == 0xC0) {
        if ((s[1] & 0xC0) == 0x80) {
            *codepoint = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
            return 2;
        }
    } else if ((s[0] & 0xF0) == 0xE0) {
        if ((s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80) {
            *codepoint = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
            return 3;
        }
    } else if ((s[0] & 0xF8) == 0xF0) {
        if ((s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80 && (s[3] & 0xC0) == 0x80) {
            *codepoint = ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
            return 4;
        }
    }

    /* Invalid UTF-8 byte, advance 1 */
    *codepoint = '?';
    return 1;
}

/*
 * Draw UTF-8 string
 */
void font_draw_string(SDL_Surface *surface, int x, int y, const char *utf8_str, Uint32 color, int scale) {
    if (!surface || !utf8_str) return;

    int cur_x = x;
    const char *p = utf8_str;

    while (*p != '\0') {
        uint32_t cp = 0;
        int bytes = decode_utf8(p, &cp);
        if (bytes == 0) break;

        font_draw_char(surface, cur_x, y, cp, color, scale);
        cur_x += 8 * scale;
        p += bytes;
    }
}

/*
 * Calculate string pixel width
 */
int font_get_string_width(const char *utf8_str, int scale) {
    if (!utf8_str) return 0;
    int count = 0;
    const char *p = utf8_str;
    while (*p != '\0') {
        uint32_t cp = 0;
        int bytes = decode_utf8(p, &cp);
        if (bytes == 0) break;
        count++;
        p += bytes;
    }
    return count * 8 * scale;
}
