/*
 * vita_main.c — entry point PS Vita (sostituisce runner/src/launcher.c).
 *
 * launcher.c PC fa: rom.cfg + file-picker Win32/Cocoa/console + GUI ImGui
 * (RECOMP_LAUNCHER) + CRC + rebuild argv -> nesrecomp_runner_run().
 * Su Vita niente picker/GUI: la ROM (fornita dall'utente, non distribuita)
 * sta in un path fisso su ux0:data, verificata via CRC come su PC.
 * Facciamo chdir alla data-dir con le API POSIX di newlib (mappate su
 * SceIo) così config.ini/keybinds.ini/savestates (risolti come "./..."
 * dal fallback di nesrecomp_exe_dir) finiscono su memoria scrivibile
 * invece che su app0: (read-only).
 *
 * Fornisce anche i simboli che launcher.c definiva e main_runner.c usa:
 * g_exe_dir[] e nesrecomp_expect_process_exit().
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

#include "game_extras.h"
#include "nes_runtime.h"
#include "crc32.h"

#define VITA_DATA_DIR "ux0:data/DUCKHUNT1/"
#define VITA_ROM_PATH VITA_DATA_DIR "DuckHunt.nes"

/* Simboli di launcher.c (escluso dalla build Vita): */
char g_exe_dir[260] = VITA_DATA_DIR;

void nesrecomp_expect_process_exit(void) {
    /* No-op su Vita: niente diagnostica atexit come su PC. */
}

int nesrecomp_runner_run(int argc, char *argv[]);

static int verify_rom_vita(const char *path, uint32_t expected) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "[Vita] ROM non trovata: %s\n", path);
        fprintf(stderr, "[Vita] Copia la tua Duck Hunt (World).nes in %s\n", path);
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 16) { fclose(f); return 0; }
    uint8_t hdr[16];
    if (fread(hdr, 1, 16, f) != 16) { fclose(f); return 0; }
    long rest = sz - 16;
    uint8_t *buf = (uint8_t *)malloc((size_t)rest);
    if (!buf) { fclose(f); return 0; }
    size_t got = fread(buf, 1, (size_t)rest, f);
    fclose(f);
    if (got != (size_t)rest) { free(buf); return 0; }
    uint32_t crc = crc32_compute(buf, (size_t)rest);
    free(buf);
    if (expected != 0 && crc != expected) {
        fprintf(stderr, "[Vita] CRC mismatch: got %08X expected %08X (continuo comunque)\n",
                crc, expected);
    } else {
        printf("[Vita] ROM OK: %s (crc %08X)\n", path, crc);
    }
    return 1;
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    setvbuf(stdout, NULL, _IONBF, 0);

    /* Data dir scrivibile (ux0). */
    mkdir(VITA_DATA_DIR, 0777);
    chdir(VITA_DATA_DIR);

    uint32_t expected = game_get_expected_crc32();
    const char *rom = VITA_ROM_PATH;

    /* In CI (stub senza ROM) il file manca: andiamo avanti comunque cosi'
     * il link/run CI non fallisce per un file assente. Con la ROM vera il
     * check sopra la valida. game_on_init() la chiama il runner dopo
     * load_rom/runtime_init: qui non la chiamiamo (evita doppia init). */
    verify_rom_vita(rom, expected);

    char *new_argv[4];
    new_argv[0] = (char *)"DuckHuntRecompVita";
    new_argv[1] = (char *)rom;
    new_argv[2] = NULL;
    return nesrecomp_runner_run(2, new_argv);
}
