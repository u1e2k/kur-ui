#ifndef SOUND_H
#define SOUND_H

#include <SDL/SDL.h>

/* Sound System Lifecycle */
int sound_init(void);
void sound_close(void);

/* Trigger the Game Boy style double-tone square wave (Pikoon!) */
void sound_trigger_pikoon(void);

/* Sound mute/enable control */
void sound_set_enabled(int enabled);
int sound_is_enabled(void);

#endif /* SOUND_H */
