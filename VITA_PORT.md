# Nota port PS Vita — i 5 punti critici

## 1. Build: da CMake/MSVC a toolchain VitaSDK (`arm-vita-eabi-gcc`)

- Upstream: `cmake -G "Visual Studio 17 2022"` + `ws2_32` + copia `SDL2.dll`
  + launcher ImGui (`recomp-ui/recomp_ui.cmake`, `recomp_target_launcher_ui`).
- Vita (`CMakeLists.txt` qui):
  `cmake -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake`,
  niente flag MSVC, niente `ws2_32`/`SDL2.dll`, niente `recomp-ui`
  (ImGui/OpenGL non disponibile su Vita).
- Link Vita: `SDL2` (vdpm) + stub SCE:
  `SceDisplay/Ctrl/Touch/Audio/Gxm/CommonDialog/Power/Hid/Motion/AppMgr/Iofilemgr`
  + `m c`. Macro `$VITASDK/share/vita.cmake`:
  `vita_create_self` / `vita_create_vpk` (vedi punto 6).
- ROM: il C ricompilato si genera **su PC host** con `nesrecomp.exe`
  (non esiste un recompiler per Vita); la build Vita incrocia solo quel C.

## 2. SDL2 per Vita (vdpm): copertura di `main_runner.c` / `ppu_renderer.c`

Verifica funzione per funzione (SDL2-Vita upstream, `vdpm install sdl2`):

| Uso nel runner | Stato su Vita |
|---|---|
| `SDL_Init`, `CreateWindow/Renderer/Texture`, `UpdateTexture`, `RenderClear/Copy/Present`, `RenderSetLogicalSize`, `GetTicks`, `Delay`, `PollEvent`, `OpenAudioDevice/PauseAudioDevice`, `GameController open/button/axis`, `SetHint`, `GetKeyboardState`, `GetScancodeFromName` | **OK** |
| `GetWindowSize/GetRendererOutputSize` | OK ma costanti (960x544): il calcolo letterbox dello Zapper è fissato in `zapper_touch.c`, senza DPI scaling |
| `SetWindowSize/SetWindowPosition/GetCurrentDisplayMode/SetWindowFullscreen/GetWindowFlags`, `ShowCursor`, `RenderSetIntegerScale` | **No-op / da evitare** (`#ifdef __VITA__` o rimossi): una sola finestra 960x544, niente cursore OS |
| Seconda finestra OAM-debug / RAM-watch (`s_debug`) | **Disattivata**: `s_debug=0` su Vita (una sola finestra supportata) |
| `GetMouseState` (Zapper) | **Sostituita** da `SDL_FINGER*` (punto 5); hint `TOUCH_MOUSE_EVENTS=0` |
| Tastiera (menu Tab/Enter, savestate F1-F12) | Assente: P1 da `SDL_GameController` (pad Vita), menu con Select/Start fisici |
| Audio `QueueAudio`/callback 44100 mono, mutex | OK; su Vita buffer 1024 consigliato contro gli underrun |

Riferimento: `docs/README-vita.md` di SDL2 —
"SDL emits mouse events for touch… use `SDL_SetHint(TOUCH_MOUSE_EVENTS,"0")`
and handle touch events instead"; touch separati con
`VITA_DISABLE_TOUCH_FRONT/BACK`.

## 3. `ucontext.h` (makecontext/swapcontext) nel backend POSIX — riscritto

- `runner/src/coroutine.c` ha due backend: Windows Fibers + POSIX `ucontext`
  (`getcontext/makecontext/swapcontext`, stack privato 1 MB/canale).
- newlib di VitaSDK su ARM **non fornisce un ucontext utilizzabile**
  (header assente o stub `ENOSYS`; comunque incompatibile con i thread
  SceKernel): la link fallirebbe con
  `undefined reference to makecontext/swapcontext`.
- Soluzione: `vita/coroutine_vita.c` — backend **sincrono senza stack
  separato** (stessa API di `coroutine.h`): `start` = `call_by_address`
  diretto, `yield` = no-op, `resume` = skip canale morto.
- Sicuro per Duck Hunt (mapper NROM-0): non usa scheduler cooperativi 6502,
  quindi nessuna coroutine reale nel path di gioco. Giochi con scheduler
  veri richiederebbero fiber SceKernel/libucontext ARM (fuori scopo).

## 4. `debug_server.c` (TCP debug server) — rimosso/disattivato

- Upstream `runner.cmake`: `NESRECOMP_ENABLE_TRACE=ON` compila
  `debug_server.c` (socket listener + ring 36000 frame + protocollo JSON).
- Su Vita: `set(NESRECOMP_ENABLE_TRACE OFF)` → compila
  `debug_server_stub.c` (stessi simboli, tutti no-op): **niente socket,
  niente porte, niente ring** (decine di MB di RAM risparmiati).
- `vita/extras_vita.c` non chiama `debug_server_init/poll/record`;
  `game_handle_debug_cmd` ritorna 0. Il link col resto del runner resta
  intatto grazie allo stub.

## 5. `extras.c`: input Zapper da mouse a touchscreen anteriore

- Originale: `SDL_GetMouseState` + `zapper_mouse_to_nes()` (DPI + letterbox
  da `GetWindowSize`), click sinistro = grilletto; crosshair + cursore nascosto.
- Vita (`vita/zapper_touch.c` + `vita/vita_input.c` + `game_on_frame` in
  `extras_vita.c`): poll `SDL_FINGERDOWN/MOTION/UP` con `touchId==0`
  (frontale; il posteriore è ignorato), coordinate normalizzate → NES 256x240
  con letterbox fisso 960x544, **tocco = mira + sparo** (`trigger=1` a dito
  giù, debounce ≥2 frame per la sequenza anti-cheat + flash del gioco).
  Crosshair 5x5 disegnato in `game_post_render` (bianco/rosso).
- Dettagli SDL-Vita: due touchscreen → `SDL_SetHint(TOUCH_MOUSE_EVENTS,"0")`
  e gestione finger diretta; disattivabili via
  `VITA_DISABLE_TOUCH_FRONT/BACK`.

## 6. Packaging in `.vpk` (`vita-mksfoex` + `vita-make-fself`)

Automatico via CMake (`vita_create_self`/`vita_create_vpk`, TITLEID
`DUCKHUNT1`), oppure manuale come in `vita/package.sh`:

```sh
cmake -S . -B build-vita \
  -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-vita

vita-mksfoex -s DUCKHUNT1 "Duck Hunt Recomp" build-vita/param.sfo
vita-make-fself -s build-vita/DuckHuntRecompVita build-vita/eboot.bin
(cd build-vita && zip -j DuckHuntRecompVita.vpk eboot.bin param.sfo)
```

Installare il `.vpk` (VitaShell), copiare la ROM in
`ux0:data/DUCKHUNT1/DuckHunt.nes`, avviare la bolla.
