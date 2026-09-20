#ifndef BOOT_H
#define BOOT_H

#include <SDL/SDL.h>

/* Initializes boot sequence parameters */
void boot_init(void);

/*
 * Updates animation state.
 * Returns 1 when boot sequence is fully completed and ready for main menu, 0 otherwise.
 */
int boot_update(void);

/* Draws the boot sequence frame */
void boot_draw(SDL_Surface *surface);

#endif /* BOOT_H */
