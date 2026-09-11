/*
 * stub_generated.c — stub del codice ricompilato per la CI Vita.
 *
 * Il codice vero (duck-hunt_full.c / duck-hunt_dispatch.c) si genera su PC
 * host con nesrecomp.exe dalla ROM originale (non distribuibile) e NON è
 * presente in CI. Questo stub fornisce gli stessi simboli (func_RESET/NMI/IRQ,
 * call_by_address*, globali dispatch/stack) così il link con
 * arm-vita-eabi-gcc riesce e la build GitHub resta verde. La build con la
 * ROM vera sovrascrive lo stub (vedi CMakeLists.txt: se generated/ esiste,
 * usa quello).
 *
 * DIAGNOSTICA VIDEO: func_RESET() mostra barre colorate ANIMATE tramite la
 * vera pipeline SDL (runner_present_framebuf). Serve a distinguere i due casi
 * di "schermo nero" su Vita:
 *   - Vedi le barre animate  => renderer/present OK, manca solo il codice
 *     generato dalla ROM (build di test CI). Ricompila con generated/ vero.
 *   - Resta nero anche qui  => problema nel renderer SDL Vita (flag
 *     SDL_CreateRenderer, vedi main_runner.c), non nel gioco.
 */
#include "nes_runtime.h"
#include <stdint.h>
#include <stdio.h>
#include <SDL.h>

/* Definita in runner/src/main_runner.c (non statica). */
extern void runner_present_framebuf(const uint32_t *argb_buf);
extern int g_render_width;

/* ---- Globali definiti dal codice generato vero ---- */
uint16_t g_rts_target = 0;
uint16_t g_rti_target = 0;
uint16_t g_rti_source = 0;
int      g_rti_bank = 0;
int      g_recomp_push_all_jsr = 0;

/* Test pattern 512x240 (stride = g_render_width, default 256). */
static uint32_t s_testbuf[512 * 240];

void func_RESET(void) {
    /* Loop di test video: barre RGB animate + bordo bianco (mostra anche
     * il letterbox). NON ritorna, come il func_RESET vero. */
    printf("[stub] Nessun codice generato: test pattern video (build CI)\n");
    uint32_t frame = 0;
    for (;;) {
        int w = (g_render_width > 0 && g_render_width <= 512)
                    ? g_render_width : 256;
        for (int y = 0; y < 240; y++) {
            for (int x = 0; x < w; x++) {
                int bar = (x + (int)(frame * 2)) / 32 % 3;
                uint32_t c = (bar == 0) ? 0xFFFF2222u
                             : (bar == 1) ? 0xFF22FF22u : 0xFF2222FFu;
                if (x < 3 || x > w - 4 || y < 3 || y > 236)
                    c = 0xFFFFFFFFu; /* bordo bianco */
                s_testbuf[y * w + x] = c;
            }
        }
        runner_present_framebuf(s_testbuf);

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT)
                return;
        }
        SDL_Delay(33);
        frame++;
    }
}

void func_NMI(void) {
    /* Nessun-op: frame vuoto per la CI. */
}

void func_IRQ(void) {
    /* Nessun-op: Duck Hunt NROM non usa IRQ; stub per la CI. */
}

int call_by_address(uint16_t addr) {
    (void)addr;
    return 0;
}

int call_by_address_cb(uint16_t addr, int caller_bank) {
    (void)addr; (void)caller_bank;
    return 0;
}
/* NOTA: call_by_address_tail e' definita in runner/src/runtime.c,
 * quindi lo stub NON la ridefinisce (multiple definition). */
