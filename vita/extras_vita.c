/*
 * extras_vita.c — hook game-specifici per il port PS Vita (sostituisce extras.c).
 *
 * DIFFERENZE rispetto a extras.c PC:
 *  - NIENTE debug_server TCP (debug_server_init/poll/record): su Vita il
 *    runner e' compilato con NESRECOMP_ENABLE_TRACE=OFF quindi linka
 *    debug_server_stub.c (no-op). Qui non chiamiamo proprio il server:
 *    niente porte, niente ring da 36000 frame, RAM risparmiata.
 *  - NIENTE verify_mode / Nestopia oracle (solo sviluppo PC).
 *  - NIENTE emulated mode: sempre func_RESET() nativo.
 *  - Zapper: abilitato su porta 2 come originale, ma mira+grilletto dal
 *    touchscreen anteriore via vita_input_poll_zapper() (ogni frame).
 *    Il crosshair e' disegnato dal runner; qui aggiorniamo solo g_zapper_*.
 */
#include "game_extras.h"
#include "nes_runtime.h"
#include "config.h"
#include "vita_input.h"
#include "zapper_touch.h"

#include <stdio.h>
#include <string.h>

/* Simboli attesi dal framework (come extras.c originale). */
const char *g_rom_path_for_extras = NULL;
int         g_watchdog_triggered  = 0;
uint32_t    g_watchdog_frame      = 0;
const char *g_watchdog_stack_dump = "";

uint32_t game_get_expected_crc32(void) { return 0x24598791u; }

const char *game_get_name(void) { return "Duck Hunt"; }

void game_on_init(void) {
    /* Niente debug_server_init(): stub no-op su Vita. */
    g_zapper_enabled = 1;
    g_zapper_x = 128;
    g_zapper_y = 120;
    g_zapper_trigger = 0;

    vita_input_init();
    /* P1 da gamepad (pad Vita via SDL_GameController nel runner): la
     * tastiera PC non esiste su Vita (default upstream {1,2} = P1 tastiera
     * = D-Pad morto). P2 resta gamepad. */
    g_nes_config.player_src[0] = 2;
    g_nes_config.player_src[1] = 2;
    printf("[Vita] Duck Hunt: Zapper su touch frontale (tocco = mira+sparo)\n");
}

void game_on_frame(uint64_t frame_count) {
    (void)frame_count;
    /* Poll touch ANTERIORE: aggiorna mira + grilletto. Sovrascrive il poll
     * mouse di main_runner (che su Vita con TOUCH_MOUSE_EVENTS=0 legge
     * 0,0/nessun bottone, quindi non sporca). */
    vita_input_poll_zapper();
    vita_zapper_state_t z = zapper_touch_get();
    g_zapper_x = z.x;
    g_zapper_y = z.y;
    g_zapper_trigger = z.trigger;
}

void game_post_nmi(uint64_t frame_count) { (void)frame_count; }

int game_handle_arg(const char *key, const char *val) {
    (void)key; (void)val;
    return 0; /* nessun flag custom su Vita */
}

const char *game_arg_usage(void) { return NULL; }

void game_run_nmi(void) {
    func_NMI(); /* sempre nativo, niente verify/emulato */
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
    /* Crosshair: lo disegna gia' il runner PC se keybinds crosshair=1.
     * Su Vita il framebuffer e' nostro: disegno minimale 5x5 qui per non
     * dipendere dai keybinds (bianco fermo, rosso in sparo). */
    extern int g_render_width;
    int cx = g_zapper_x, cy = g_zapper_y;
    if (cx < 2 || cx > g_render_width - 3 || cy < 2 || cy > 237) return;
    uint32_t col = g_zapper_trigger ? 0xFFFF2222u : 0xFFFFFFFFu;
    for (int dy = -2; dy <= 2; dy++) {
        framebuf[(cy + dy) * g_render_width + cx] = col;
    }
    for (int dx = -2; dx <= 2; dx++) {
        framebuf[cy * g_render_width + (cx + dx)] = col;
    }
}

int game_handle_debug_cmd(const char *cmd, int id, const char *json) {
    (void)cmd; (void)id; (void)json;
    return 0; /* nessun server TCP su Vita */
}
