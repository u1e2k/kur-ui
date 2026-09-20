#include "sound.h"
#include <string.h>

#define SAMPLE_RATE 22050
#define BUFFER_SIZE 256

/* Frequencies for C6 and C7 */
#define FREQ_C6 1046.5f
#define FREQ_C7 2093.0f

#define TONE1_SAMPLES (int)(SAMPLE_RATE * 0.085f) /* ~85ms */
#define TONE2_SAMPLES (int)(SAMPLE_RATE * 0.320f) /* ~320ms */

#define MAX_AMPLITUDE 48

typedef enum {
    TONE_IDLE = 0,
    TONE_PART1,
    TONE_PART2
} ToneState;

static volatile ToneState g_tone_state = TONE_IDLE;
static int g_sample_idx = 0;
static uint32_t g_phase = 0;
static uint32_t g_phase_step1 = 0;
static uint32_t g_phase_step2 = 0;
static int g_sound_active = 0;

/*
 * SDL Audio Callback
 * Runs in a separate audio thread. Generates 8-bit unsigned square wave samples.
 */
static void audio_callback(void *userdata, Uint8 *stream, int len) {
    (void)userdata;

    if (g_tone_state == TONE_IDLE) {
        memset(stream, 128, len); /* 128 is center (silence) in AUDIO_U8 */
        return;
    }

    for (int i = 0; i < len; ++i) {
        Uint8 out = 128;

        if (g_tone_state == TONE_PART1) {
            g_phase += g_phase_step1;
            int amp = MAX_AMPLITUDE;
            /* 50% duty square wave */
            int sign = (g_phase & 0x80000000) ? 1 : -1;
            out = (Uint8)(128 + sign * amp);

            g_sample_idx++;
            if (g_sample_idx >= TONE1_SAMPLES) {
                g_tone_state = TONE_PART2;
                g_sample_idx = 0;
                g_phase = 0;
            }
        } else if (g_tone_state == TONE_PART2) {
            g_phase += g_phase_step2;
            /* Linear decay envelope */
            int remaining = TONE2_SAMPLES - g_sample_idx;
            if (remaining < 0) remaining = 0;
            int amp = (MAX_AMPLITUDE * remaining) / TONE2_SAMPLES;

            int sign = (g_phase & 0x80000000) ? 1 : -1;
            out = (Uint8)(128 + sign * amp);

            g_sample_idx++;
            if (g_sample_idx >= TONE2_SAMPLES) {
                g_tone_state = TONE_IDLE;
                g_sample_idx = 0;
                g_phase = 0;
            }
        }

        stream[i] = out;
    }
}

int sound_init(void) {
    if (g_sound_active) return 0;

    SDL_AudioSpec desired, obtained;
    memset(&desired, 0, sizeof(desired));
    desired.freq = SAMPLE_RATE;
    desired.format = AUDIO_U8;
    desired.channels = 1;
    desired.samples = BUFFER_SIZE;
    desired.callback = audio_callback;
    desired.userdata = NULL;

    if (SDL_OpenAudio(&desired, &obtained) < 0) {
        return -1;
    }

    /* Pre-calculate phase steps for Q32 phase accumulator */
    /* step = (frequency / sample_rate) * 2^32 */
    double step1 = (double)FREQ_C6 / (double)obtained.freq * 4294967296.0;
    double step2 = (double)FREQ_C7 / (double)obtained.freq * 4294967296.0;
    g_phase_step1 = (uint32_t)step1;
    g_phase_step2 = (uint32_t)step2;

    g_tone_state = TONE_IDLE;
    g_sample_idx = 0;
    g_phase = 0;
    g_sound_active = 1;

    SDL_PauseAudio(0); /* Unpause audio */
    return 0;
}

void sound_close(void) {
    if (!g_sound_active) return;
    SDL_PauseAudio(1);
    SDL_CloseAudio();
    g_sound_active = 0;
    g_tone_state = TONE_IDLE;
}

void sound_trigger_pikoon(void) {
    if (!g_sound_active) return;

    SDL_LockAudio();
    g_tone_state = TONE_PART1;
    g_sample_idx = 0;
    g_phase = 0;
    SDL_UnlockAudio();
}
