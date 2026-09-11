/*
 * perf_vita.c — vedi perf_vita.h. Tutto basato su SDL_GetTicks (ms).
 * Nessuna allocazione, un flush ogni 300 frame.
 */
#include "perf_vita.h"

#include <SDL.h>
#include <stdio.h>

static FILE *s_pf = NULL;
static uint32_t s_last_frame_tick = 0;
static uint32_t s_sum_dt = 0, s_max_dt = 0;
static uint32_t s_sum_nmi = 0, s_max_nmi = 0;
static uint32_t s_sum_snap_ms = 0, s_max_snap_ms = 0;
static uint32_t s_sum_snaps = 0, s_max_snaps = 0;
static uint32_t s_frames = 0;
static uint32_t s_cur_snaps = 0;
static uint32_t s_snap_t0 = 0;
static int s_mouse_seen = 0, s_finger_seen = 0;
static int s_header_done = 0;

void perf_init(void) {
    if (s_pf) return;
    s_pf = fopen("perf.log", "w");
    s_last_frame_tick = SDL_GetTicks();
}

void perf_frame(void) {
    if (!s_pf) perf_init();
    uint32_t now = SDL_GetTicks();
    uint32_t dt = now - s_last_frame_tick;
    s_last_frame_tick = now;
    s_sum_dt += dt;
    if (dt > s_max_dt) s_max_dt = dt;
    /* chiudi il conteggio snapshot del frame precedente */
    if (s_frames > 0) {
        s_sum_snaps += s_cur_snaps;
        if (s_cur_snaps > s_max_snaps) s_max_snaps = s_cur_snaps;
    }
    s_cur_snaps = 0;
    s_frames++;
    if ((s_frames % 300) == 0) {
        uint32_t n = 300;
        fprintf(s_pf,
                "frames=%u avg_dt_ms=%u max_dt_ms=%u | nmi avg=%u max=%u | "
                "snap/frame avg=%u max=%u snap_ms avg=%u max=%u | mouse=%d finger=%d\n",
                (unsigned)s_frames,
                (unsigned)(s_sum_dt / n), (unsigned)s_max_dt,
                (unsigned)(s_sum_nmi / n), (unsigned)s_max_nmi,
                (unsigned)(s_sum_snaps / n), (unsigned)s_max_snaps,
                (unsigned)(s_sum_snap_ms / n), (unsigned)s_max_snap_ms,
                s_mouse_seen, s_finger_seen);
        fflush(s_pf);
        s_sum_dt = s_sum_nmi = s_sum_snap_ms = s_sum_snaps = 0;
        s_max_dt = s_max_nmi = s_max_snap_ms = s_max_snaps = 0;
    }
    (void)s_header_done;
}

uint32_t perf_nmi_begin(void) {
    return SDL_GetTicks();
}

void perf_nmi_end(uint32_t t0) {
    uint32_t d = SDL_GetTicks() - t0;
    s_sum_nmi += d;
    if (d > s_max_nmi) s_max_nmi = d;
}

void perf_snap_begin(void) {
    s_snap_t0 = SDL_GetTicks();
}

void perf_snap_end(void) {
    uint32_t d = SDL_GetTicks() - s_snap_t0;
    s_sum_snap_ms += d;
    if (d > s_max_snap_ms) s_max_snap_ms = d;
}

void perf_snap_called(void) {
    s_cur_snaps++;
}

void perf_note_mouse(int buttons) {
    if (buttons) s_mouse_seen = 1;
}

void perf_note_finger(void) {
    s_finger_seen = 1;
}
