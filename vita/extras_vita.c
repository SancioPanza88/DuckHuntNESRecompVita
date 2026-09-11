/*
 * extras_vita.c — hook game-specifici per il port PS Vita (sostituisce extras.c).
 *
 * DIFFERENZE rispetto a extras.c PC:
 *  - NIENTE debug_server TCP (stub no-op, NESRECOMP_ENABLE_TRACE=OFF).
 *  - NIENTE verify_mode / Nestopia oracle, sempre func_RESET() nativo.
 *  - Zapper su porta 2 via touchscreen anteriore (vedi sotto).
 *  - Snapshot Zapper CACHATO (1 render/frame max): il runner di default
 *    ri-renderizza l'intero frame software a OGNI lettura $4017; su CPU Vita
 *    sono millisecondi a botta e il gioco legge spesso => crollo fps.
 *    Duck Hunt non fa split mid-frame (confermato upstream), quindi la cache
 *    per-frame e' sicura. Risparmio tipico: N-1 render pieni a frame.
 */
#include "game_extras.h"
#include "nes_runtime.h"   /* func_NMI/RESET, g_zapper_*, runtime_set_zapper_* */
#include "ppu_dot.h"       /* g_dot_ppu_on, ppu_dot_render_snapshot */
#include "config.h"
#include "vita_input.h"
#include "zapper_touch.h"
#include "perf_vita.h"

#include <SDL.h>
#include <stdio.h>
#include <string.h>

/* Simboli attesi dal framework (come extras.c originale). */
const char *g_rom_path_for_extras = NULL;
int         g_watchdog_triggered  = 0;
uint32_t    g_watchdog_frame      = 0;
const char *g_watchdog_stack_dump = "";

/* Buffer snapshot privato (512x240 max, come il runner). */
static uint32_t s_vita_snap[512 * 240];
static uint64_t s_snap_frame = (uint64_t)-1;

/* Render on-demand con cache per-frame: la prima lettura $4017 del frame
 * renderizza, le successive riusano. Registrata ogni frame in game_on_frame
 * (il runner registra la sua solo all'avvio). */
static void vita_zapper_snap_cached(void) {
    perf_snap_called();
    if (s_snap_frame == g_frame_count) return; /* cache hit */
    s_snap_frame = g_frame_count;
    perf_snap_begin();
    if (g_dot_ppu_on)
        ppu_dot_render_snapshot(s_vita_snap);
    else
        ppu_render_frame(s_vita_snap);
    runtime_set_zapper_snapshot(s_vita_snap);
    perf_snap_end();
}

uint32_t game_get_expected_crc32(void) { return 0x24598791u; }

const char *game_get_name(void) { return "Duck Hunt"; }

void game_on_init(void) {
    /* Niente debug_server_init(): stub no-op su Vita. */
    g_zapper_enabled = 1;
    g_zapper_x = 128;
    g_zapper_y = 120;
    g_zapper_trigger = 0;

    vita_input_init(); /* hint TOUCH_MOUSE_EVENTS=1: il touch emula il mouse */
    perf_init();
    /* P1 da gamepad (pad Vita via SDL_GameController nel runner): la
     * tastiera PC non esiste su Vita (default upstream {1,2} = P1 tastiera
     * = D-Pad morto). P2 resta gamepad. */
    g_nes_config.player_src[0] = 2;
    g_nes_config.player_src[1] = 2;
    printf("[Vita] Duck Hunt: Zapper su touch frontale (tocco = mira+sparo)\n");
}

void game_on_frame(uint64_t frame_count) {
    (void)frame_count;
    perf_frame();

    /* Lo snapshot cachato vale da questo frame in poi. */
    runtime_set_zapper_render_callback(vita_zapper_snap_cached);

    /* Input Zapper: il main_runner ha appena drenato la coda eventi e, con
     * l'emulazione mouse attiva, ha gia' impostato mira+trigger freschi da
     * SDL_GetMouseState (nessuna race: e' level, non edge). Qui usiamo il
     * ground truth del bottone mouse: se premuto, i valori del runner sono
     * giusti e non li tocchiamo; se rilasciato, forziamo trigger=0 (copre
     * anche l'UP perso) e teniamo l'ultima mira. Solo se l'emulazione mouse
     * e' disattivata usiamo gli eventi finger diretti. */
    {
        const char *h = SDL_GetHint(SDL_HINT_TOUCH_MOUSE_EVENTS);
        int mouse_emu = (!h || h[0] == '1');
        if (mouse_emu) {
            int mx = 0, my = 0;
            Uint32 mb = SDL_GetMouseState(&mx, &my);
            perf_note_mouse((int)(mb & SDL_BUTTON_LMASK));
            if (!(mb & SDL_BUTTON_LMASK)) {
                g_zapper_trigger = 0; /* dito alzato: niente sparo fantasma */
            }
            /* Se premuto: mira+trigger del runner già corretti, non toccare. */
        } else {
            vita_input_poll_zapper();
            vita_zapper_state_t z = zapper_touch_get();
            g_zapper_x = z.x;
            g_zapper_y = z.y;
            g_zapper_trigger = z.trigger;
        }
    }
}

void game_post_nmi(uint64_t frame_count) { (void)frame_count; }

int game_handle_arg(const char *key, const char *val) {
    (void)key; (void)val;
    return 0; /* nessun flag custom su Vita */
}

const char *game_arg_usage(void) { return NULL; }

void game_run_nmi(void) {
    uint32_t t0 = perf_nmi_begin();
    func_NMI(); /* sempre nativo, niente verify/emulato */
    perf_nmi_end(t0);
}

void game_run_main(void) {
    func_RESET(); /* mai ritorna */
}

int game_dispatch_override(uint16_t addr) {
    (void)addr;
    return 0;
}

uint8_t game_ram_read_hook(uint16_t pc, uint16_t addr, uint8_t val) {
    (void)pc; (void)addr;
    return val;
}

void game_fill_frame_record(void *record) { (void)record; }

void game_post_render(uint32_t *framebuf) {
    (void)framebuf;
    /* Il crosshair lo disegna gia' il runner (keybinds crosshair=1):
     * niente doppio disegno qui. */
}

int game_handle_debug_cmd(const char *cmd, int id, const char *json) {
    (void)cmd; (void)id; (void)json;
    return 0; /* nessun server TCP su Vita */
}
