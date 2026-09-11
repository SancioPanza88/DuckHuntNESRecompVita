#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Logger performance minimale su ux0:data/DUCKHUNT1/perf.log (una riga ogni
 * 300 frame + conteggi input). Serve a capire su hardware reale dove vanno
 * i millisecondi (NMI generato? snapshot Zapper? altro?). Flush raro: zero
 * impatto sul frame rate. */
void perf_init(void);
void perf_frame(void);          /* inizio frame (in game_on_frame) */
uint32_t perf_nmi_begin(void);  /* ritorna tick inizio */
void perf_nmi_end(uint32_t t0);
void perf_snap_begin(void);     /* idem per lo snapshot Zapper */
void perf_snap_end(void);
void perf_snap_called(void);    /* conteggio render snapshot per frame */
void perf_note_mouse(int buttons);   /* osservato stato mouse (emulazione?) */
void perf_note_finger(void);         /* osservato evento finger */

#ifdef __cplusplus
}
#endif
