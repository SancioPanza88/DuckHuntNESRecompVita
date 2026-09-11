/*
 * stub_generated.c — stub del codice ricompilato per la CI Vita.
 *
 * Il codice vero (duck-hunt_full.c / duck-hunt_dispatch.c) si genera su PC
 * host con nesrecomp.exe dalla ROM originale (non distribuibile) e NON è
 * presente in CI. Questo stub fornisce gli stessi simboli (func_RESET,
 * func_NMI, call_by_address) così il link con arm-vita-eabi-gcc riesce e la
 * build GitHub resta verde. La build con la ROM vera sovrascrive lo stub
 * (vedi CMakeLists.txt: se generated/ esiste, usa quello).
 */
#include "nes_runtime.h"
#include <stdio.h>

void func_RESET(void) {
#ifdef __VITA__
    /* Su hardware reale non si arriva qui con la ROM vera. Nello stub CI
     * restiamo in un loop innocuo invece di uscire. */
    printf("[stub] func_RESET: nessuna ROM generata (build CI senza ROM)\n");
    for (;;) {
        /* Il runner chiama comunque il vblank; qui basta non crashare. */
    }
#else
    printf("[stub] func_RESET: nessuna ROM generata (build CI senza ROM)\n");
#endif
}

void func_NMI(void) {
    /* Nessun-op: frame vuoto per la CI. */
}

int call_by_address(uint16_t addr) {
    (void)addr;
    return 0;
}
