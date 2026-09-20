#ifndef FONT_H
#define FONT_H

#include <SDL/SDL.h>
#include <stdint.h>

/* Pixel and Shape Drawing Helpers */
void draw_pixel(SDL_Surface *surface, int x, int y, Uint32 color);
void draw_fill_rect(SDL_Surface *surface, int x, int y, int w, int h, Uint32 color);
void draw_line_h(SDL_Surface *surface, int x, int y, int w, Uint32 color);
void draw_line_v(SDL_Surface *surface, int x, int y, int h, Uint32 color);
void draw_rect(SDL_Surface *surface, int x, int y, int w, int h, Uint32 color);

/* Font and String Drawing */
void font_draw_char(SDL_Surface *surface, int x, int y, uint32_t codepoint, Uint32 color, int scale);
void font_draw_string(SDL_Surface *surface, int x, int y, const char *utf8_str, Uint32 color, int scale);
int font_get_string_width(const char *utf8_str, int scale);

#endif /* FONT_H */
