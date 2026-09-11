#pragma once
#include <stdint.h>

/* Stato Zapper letto dal touchscreen anteriore (SDL_FINGER* / SceTouch). */
typedef struct {
    int x;          /* 0-255 (NES) */
    int y;          /* 0-239 (NES) */
    int trigger;    /* 1 = dito giu' (sparo) */
    int active;     /* 1 = dito rilevato questo frame */
} vita_zapper_state_t;

#ifdef __cplusplus
extern "C" {
#endif

/* Converte coordinate touch normalizzate SDL (0.0-1.0, area 960x544) in NES. */
void zapper_touch_to_nes(float tx, float ty, int *out_x, int *out_y);

/* Ultimo stato (scritto da vita_input_poll, letto da extras_vita). */
vita_zapper_state_t zapper_touch_get(void);

/* Chiamato dal poll input con le coordinate normalizzate del dito frontale.
 * touch_down=1 mentre il dito e' giu' (mira+sparo), 0 al rilascio. */
void zapper_touch_update(float tx, float ty, int touch_down);

/* Nessun tocco questo frame: rilascia il trigger. */
void zapper_touch_clear(void);

#ifdef __cplusplus
}
#endif
