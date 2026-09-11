/*
 * zapper_touch.c — Zapper da mouse a touchscreen anteriore (PS Vita).
 *
 * ORIGINALE (PC): extras.c + main_runner.c usano SDL_GetMouseState() e
 * zapper_mouse_to_nes(): movimento mouse = mira, click sinistro = grilletto.
 * Il cursore OS e' nascosto e un crosshair e' disegnato sul framebuffer
 * (bianco normale, rosso durante lo sparo).
 *
 * VITA: niente mouse. Usiamo il touchscreen ANTERIORE (960x544):
 *   tocco = mira + sparo insieme (premi dove vuoi colpire).
 * Il runner SDL-Vita invia SDL_FINGERDOWN/MOTION/UP con coordinate
 * normalizzate 0.0-1.0; il backend SceTouch e' il touchId 0 (frontale).
 * Teniamo il dito giu' per almeno 2 frame cosi' la sequenza di detection del
 * gioco (check anti-cheat a schermo nero + flash di detection) vede il
 * trigger premuto al momento giusto, come il click del mouse.
 *
 * Coordinate: lo schermo Vita e' 960x544, il NES e' 256x240 con letterbox
 * (stesso calcolo di zapper_mouse_to_nes ma da area fissa, senza
 * SDL_GetWindowSize che su Vita e' costante). Clamp 0-255 / 0-239.
 */
#include "zapper_touch.h"

/* Risoluzione display Vita e area di rendering NES letterboxed. */
#define VITA_DISP_W 960
#define VITA_DISP_H 544
#define NES_W 256
#define NES_H 240

static vita_zapper_state_t s_state = { 128, 120, 0, 0 };
static int s_hold_frames = 0; /* debounce: mantieni il trigger >= 2 frame */

void zapper_touch_to_nes(float tx, float ty, int *out_x, int *out_y) {
    if (tx < 0.0f) tx = 0.0f;
    if (tx > 1.0f) tx = 1.0f;
    if (ty < 0.0f) ty = 0.0f;
    if (ty > 1.0f) ty = 1.0f;

    float px = tx * (float)VITA_DISP_W;
    float py = ty * (float)VITA_DISP_H;

    /* Letterbox: scala uniforme, centarta. */
    float scale_x = (float)VITA_DISP_W / (float)NES_W;
    float scale_y = (float)VITA_DISP_H / (float)NES_H;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;
    float render_w = (float)NES_W * scale;
    float render_h = (float)NES_H * scale;
    float offset_x = ((float)VITA_DISP_W - render_w) / 2.0f;
    float offset_y = ((float)VITA_DISP_H - render_h) / 2.0f;

    int nes_x = (int)((px - offset_x) * (float)NES_W / render_w);
    int nes_y = (int)((py - offset_y) * (float)NES_H / render_h);
    if (nes_x < 0) nes_x = 0;
    if (nes_x > 255) nes_x = 255;
    if (nes_y < 0) nes_y = 0;
    if (nes_y > 239) nes_y = 239;

    if (out_x) *out_x = nes_x;
    if (out_y) *out_y = nes_y;
}

vita_zapper_state_t zapper_touch_get(void) {
    return s_state;
}

void zapper_touch_update(float tx, float ty, int touch_down) {
    int nx, ny;
    zapper_touch_to_nes(tx, ty, &nx, &ny);
    s_state.x = nx;
    s_state.y = ny;
    if (touch_down) {
        s_state.trigger = 1;
        s_state.active = 1;
        s_hold_frames = 2; /* garantisci almeno 2 frame di sparo */
    } else {
        zapper_touch_clear();
    }
}

void zapper_touch_clear(void) {
    if (s_hold_frames > 0) {
        /* Debounce: resta premuto ancora un frame per la detection flash. */
        s_hold_frames--;
        if (s_hold_frames > 0) {
            s_state.trigger = 1;
            return;
        }
    }
    s_state.trigger = 0;
    s_state.active = 0;
}
