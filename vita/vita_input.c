/*
 * vita_input.c — poll touch frontale + pad per PS Vita (SDL2-Vita / vdpm).
 *
 * TOUCH: SDL-Vita emette SDL_FINGERMOTION/DOWN/UP. Di default SDL emula anche
 * il mouse dagli eventi touch (SDL_HINT_TOUCH_MOUSE_EVENTS=1); qui lo
 * DISABILITIAMO e gestiamo i finger direttamente, come raccomanda
 * docs/README-vita.md di SDL2 ("use SDL_SetHint(TOUCH_MOUSE_EVENTS,"0") and
 * handle touch events instead"). Il touchId 0 e' il pannello frontale
 * (il posteriore e' disabilitato via env VITA_DISABLE_TOUCH_BACK se serve).
 * Tocco frontale = mira + sparo (vedi zapper_touch.c).
 *
 * PAD: la tastiera PC non esiste su Vita. I bottoni NES P1 arrivano dal pad
 * Vita tramite SDL_GameController (Cross=A, Circle=B, Select, Start, D-Pad;
 * stick sinistro come D-Pad). La navigazione menu (Tab=Select, Enter=Start su
 * PC) diventa Select/Start fisici. Il trigger dello Zapper NON sta sul pad:
 * e' solo il tocco (evita spari accidentali).
 */
#include "vita_input.h"
#include "zapper_touch.h"

#include <SDL.h>

/* Ultimo dito frontale visto (coordinate normalizzate 0-1). */
static float s_fx = 0.5f, s_fy = 0.5f;
static int s_finger_down = 0;

void vita_input_init(void) {
    /* Gestiamo i touch a mano: niente emulazione mouse che sporca
     * SDL_GetMouseState nel main_runner. */
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
}

int vita_input_poll_zapper(void) {
    SDL_Event ev;
    int saw_up = 0;

    /* Dreniamo gli eventi touch qui; gli altri restano al main_runner
     * (SDL_PollEvent condivide la coda: qui consumiamo solo FINGER*).
     * Range DOWN..MOTION copre DOWN(0x700), UP(0x701), MOTION(0x702). */
    while (SDL_PeepEvents(&ev, 1, SDL_GETEVENT,
                          SDL_FINGERDOWN, SDL_FINGERMOTION) > 0) {
        if (ev.type == SDL_FINGERDOWN || ev.type == SDL_FINGERMOTION) {
            /* touchId 0 = frontale. Ignoriamo il posteriore (touchId != 0)
             * per non sparare appoggiando le dita dietro. */
            if (ev.tfinger.touchId == 0) {
                s_fx = ev.tfinger.x;
                s_fy = ev.tfinger.y;
                s_finger_down = 1;
                zapper_touch_update(s_fx, s_fy, 1);
            }
        } else if (ev.type == SDL_FINGERUP) {
            if (ev.tfinger.touchId == 0) {
                saw_up = 1;
            }
        }
    }
    if (saw_up && s_finger_down) {
        s_finger_down = 0;
        zapper_touch_update(s_fx, s_fy, 0);
    }
    if (!s_finger_down) {
        zapper_touch_clear();
    }
    return s_finger_down;
}

unsigned char vita_input_pad_buttons(void) {
    /* Fallback tastiera (utile solo su build PC dev / Vita3K con tastiera).
     * Il pad VERO passa dal runner: extras_vita imposta player_src[0]=2
     * cosi' main_runner legge il pad Vita via controller_read_player()
     * (handle SDL_GameController cachati, con deadzone da config). Qui NON
     * apriamo/chiudiamo controller ogni frame (costoso e in conflitto). */
    unsigned char btn = 0;
    const uint8_t *ks = SDL_GetKeyboardState(NULL);
    if (ks) {
        if (ks[SDL_SCANCODE_Z])      btn |= 0x80;
        if (ks[SDL_SCANCODE_X])      btn |= 0x40;
        if (ks[SDL_SCANCODE_TAB])    btn |= 0x20;
        if (ks[SDL_SCANCODE_RETURN]) btn |= 0x10;
        if (ks[SDL_SCANCODE_UP])     btn |= 0x08;
        if (ks[SDL_SCANCODE_DOWN])   btn |= 0x04;
        if (ks[SDL_SCANCODE_LEFT])   btn |= 0x02;
        if (ks[SDL_SCANCODE_RIGHT])  btn |= 0x01;
    }
    return btn;
}
