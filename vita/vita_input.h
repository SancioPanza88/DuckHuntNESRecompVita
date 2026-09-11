#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Inizializza hint touch + controller. Chiamare dopo SDL_Init. */
void vita_input_init(void);

/* Legge SDL_PollEvent per SDL_FINGER* (touch frontale = Zapper) e aggiorna
 * zapper_touch.c. Chiamare una volta per frame da game_on_frame().
 * Ritorna 1 se il dito e' giu' (sparo), 0 altrimenti. */
int vita_input_poll_zapper(void);

/* Mappa i tasti Vita (via SDL_GameController / joystick) sui bottoni NES P1.
 * Bitmask stile NES: 0x80 A, 0x40 B, 0x20 Select, 0x10 Start,
 * 0x08 Up, 0x04 Down, 0x02 Left, 0x01 Right. */
unsigned char vita_input_pad_buttons(void);

#ifdef __cplusplus
}
#endif
