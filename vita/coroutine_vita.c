/*
 * coroutine_vita.c — backend coroutine per PS Vita (sostituisce ucontext).
 *
 * PERCHE': il backend POSIX di runner/src/coroutine.c usa <ucontext.h>
 * (getcontext/makecontext/swapcontext) con 1 MB di stack privato per canale.
 * La newlib di VitaSDK su ARM non fornisce un ucontext funzionante
 * (header assente o stub ENOSYS; comunque incompatibile con i thread
 * SceKernel e con lo stack privato): la link con arm-vita-eabi-gcc fallirebbe
 * con "undefined reference to makecontext/swapcontext".
 *
 * COSA FA: backend sincrono senza stack separato. start/resume eseguono
 * call_by_address() direttamente sul thread corrente; yield è un no-op.
 * E' sufficiente per Duck Hunt (mapper NROM-0): non usa scheduler
 * cooperativi 6502 (nessuna chiamata coroutine nel path di gioco), quindi la
 * semantica fiber non serve. Giochi con scheduler reali richiederebbero fiber
 * SceKernel o libucontext ARM — fuori scopo per questo port.
 *
 * API identica a runner/include/coroutine.h così il resto del runner non
 * cambia.
 */
#include "coroutine.h"
#include "nes_runtime.h"
#include <stdio.h>

/* Dichiarato nel codice generato (stub in CI, vero con la ROM). */
extern int call_by_address(uint16_t addr);

static int s_active = 0;
static int s_current_channel = -1;
static int s_has_context[COROUTINE_MAX_CHANNELS] = {0};

static int s_yield_count = 0;
static int s_resume_count = 0;
static int s_start_count = 0;
static uint8_t s_last_yield_sp = 0;
static uint8_t s_last_resume_sp = 0;

static SchedTraceEntry s_sched_trace[SCHED_TRACE_SIZE];
static int s_sched_trace_count = 0;
static int s_sched_trace_idx = 0;

static void sched_trace_record(SchedEventType evt, int channel, uint16_t addr) {
    SchedTraceEntry *e = &s_sched_trace[s_sched_trace_idx];
    e->frame = g_frame_count;
    e->channel = channel;
    e->addr = addr;
    e->sp_before = g_cpu.S;
    e->event_type = (uint8_t)evt;
    s_sched_trace_idx = (s_sched_trace_idx + 1) % SCHED_TRACE_SIZE;
    if (s_sched_trace_count < SCHED_TRACE_SIZE) s_sched_trace_count++;
}

int coroutine_scheduler_setjmp(void) {
    /* Nessun setjmp reale: lo scheduler gira sul thread principale. */
    s_active = 1;
    return 0;
}

void coroutine_yield(void) {
    if (!s_active) {
        fprintf(stderr, "[coroutine:vita] yield senza scheduler attivo\n");
        return;
    }
    s_yield_count++;
    s_last_yield_sp = g_cpu.S;
    sched_trace_record(SCHED_EVT_YIELD, s_current_channel, 0);
    /* No-op: nessuna fiber da cui tornare, l'esecuzione continua. */
}

void coroutine_resume(int channel) {
    if (channel < 0 || channel >= COROUTINE_MAX_CHANNELS) {
        fprintf(stderr, "[coroutine:vita] resume canale %d non valido\n", channel);
        return;
    }
    if (!s_has_context[channel]) return; /* canale morto: skip come upstream */
    s_current_channel = channel;
    s_resume_count++;
    s_last_resume_sp = g_cpu.S;
    sched_trace_record(SCHED_EVT_RESUME, channel, 0);
    /* Backend sincrono: riesegue dall'indirizzo? Non abbiamo l'entry salvata
     * (niente stack separato): per Duck Hunt questo path non si attiva mai.
     * Manteniamo il comportamento "skip" per non introdurre regressioni. */
}

void coroutine_start(int channel, uint16_t addr) {
    if (channel < 0 || channel >= COROUTINE_MAX_CHANNELS) {
        fprintf(stderr, "[coroutine:vita] start canale %d non valido\n", channel);
        return;
    }
    s_start_count++;
    s_current_channel = channel;
    sched_trace_record(SCHED_EVT_START, channel, addr);
    s_has_context[channel] = 1;
    call_by_address(addr); /* esecuzione diretta, senza fiber */
    s_has_context[channel] = 0;
}

void coroutine_set_channel(int channel) {
    s_current_channel = channel;
}

int coroutine_is_active(void) {
    return s_active;
}

int coroutine_has_context(int channel) {
    if (channel < 0 || channel >= COROUTINE_MAX_CHANNELS) return 0;
    return s_has_context[channel];
}

void coroutine_get_debug_counters(int *yields, int *resumes, int *starts,
                                  uint8_t *yield_sp, uint8_t *resume_sp) {
    if (yields)    *yields    = s_yield_count;
    if (resumes)   *resumes   = s_resume_count;
    if (starts)    *starts    = s_start_count;
    if (yield_sp)  *yield_sp  = s_last_yield_sp;
    if (resume_sp) *resume_sp = s_last_resume_sp;
}

void coroutine_get_sched_trace(int *out_count, int *out_idx) {
    *out_count = s_sched_trace_count;
    *out_idx = s_sched_trace_idx;
}

const SchedTraceEntry *coroutine_get_sched_trace_buf(void) {
    return s_sched_trace;
}

int coroutine_get_current_channel(void) {
    return s_current_channel;
}

int coroutine_restart_requested(void) {
    return 0;
}
