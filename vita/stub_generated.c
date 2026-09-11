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
 */
#include "nes_runtime.h"
#include <stdint.h>
#include <stdio.h>

/* ---- Globali definiti dal codice generato vero ---- */
uint16_t g_rts_target = 0;
uint16_t g_rti_target = 0;
uint16_t g_rti_source = 0;
int      g_rti_bank = 0;
int      g_recomp_push_all_jsr = 0;

void func_RESET(void) {
    /* Su hardware reale non si arriva qui con la ROM vera. Nello stub CI
     * restiamo in un loop innocuo invece di uscire. */
    printf("[stub] func_RESET: nessuna ROM generata (build CI senza ROM)\n");
    for (;;) {
        /* Il runner chiama comunque il vblank; qui basta non crashare. */
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
