# Duck Hunt NES Recomp — port PS Vita (VitaSDK)

Static recompilation di Duck Hunt (NES) per PS Vita, basata su
[mstan/DuckHuntNESRecomp](https://github.com/mstan/DuckHuntNESRecomp) e sul
framework [mstan/nesrecomp](https://github.com/mstan/nesrecomp) (submodule).
Toolchain: **VitaSDK** (`arm-vita-eabi-gcc`), SDL2 per Vita (vdpm).

> Dettaglio tecnico completo dei 5 punti critici: vedi [`VITA_PORT.md`](VITA_PORT.md).

## Controlli

- `main` — build Vita (`vitasdk/vitasdk:2026.08` + `vdpm install sdl2`) verde.
- La CI compila **senza ROM** (usa `vita/stub_generated.c`): prova il toolchain,
  non distribuisce materiale coperto da copyright.

## Schermo nero? Leggi qui prima

Il `.vpk` della CI è una **build di test senza gioco** (usa
`vita/stub_generated.c`: niente ROM = niente codice). Mostra **barre
colorate animate** tramite la vera pipeline SDL:

- **Vedi le barre animate** → video OK. Per giocare devi ricompilare con il
  codice generato dalla tua ROM (vedi "Uso reale" sotto): il CMake usa
  `generated/` appena esistono `duck-hunt_full.c` + `duck-hunt_dispatch.c`.
- **Resta nero anche con le barre attese** (cioè non vedi nemmeno quelle) →
  problema nel renderer SDL Vita: apri una issue con modello Vita + firmware.
- **L'app si chiude subito** → manca la ROM in
  `ux0:data/DUCKHUNT1/DuckHunt.nes` (controlla il nome esatto, maiuscole incluse).

## Uso reale (con la tua ROM)

1. Su **PC host**: genera il C ricompilato dalla tua `Duck Hunt (World).nes`
   con il recompiler upstream, poi copia i file in `generated/`:
   `generated/duck-hunt_full.c`, `generated/duck-hunt_dispatch.c`
   (vedi README upstream + `game.toml` — qui non incluso).
2. Ricompila per Vita (vedi sotto): il CMake usa `generated/` se presente,
   altrimenti lo stub CI.
3. Copia la ROM su Vita in `ux0:data/DUCKHUNT1/DuckHunt.nes` (CRC atteso
   `0x24598791`; se diverso parte comunque con un warning).
4. Installa il `.vpk` e gioca: **tocco frontale = mira + sparo**.

## Comandi

```sh
git submodule update --init --recursive

# Dev PC (verifica logica senza VitaSDK)
cmake -S . -B build && cmake --build build

# Vita (richiede VITASDK impostato)
cmake -S . -B build-vita \
  -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-vita

# Packaging manuale in .vpk (stessi step del CMake/vita_create_vpk)
sh vita/package.sh build-vita
```

## Packaging in .vpk (riepilogo)

```sh
vita-mksfoex -s DUCKHUNT1 "Duck Hunt Recomp" build-vita/param.sfo
vita-make-fself -s build-vita/DuckHuntRecompVita build-vita/eboot.bin
(cd build-vita && zip -j DuckHuntRecompVita.vpk eboot.bin param.sfo)
```

## Controlli Vita

| Azione | Input |
|---|---|
| Mira Zapper | Tocco touchscreen anteriore (trascina) |
| Sparo Zapper | Tocco (dito giù = trigger premuto) |
| D-Pad / A / B / Start / Select | Pad Vita (Cross=A, Circle=B, D-Pad, Start/Select) + stick sinistro come D-Pad |
| Menu (Select/Start) | Tasti Select/Start fisici |

## File del port (`vita/`)

- `extras_vita.c` — hook gioco: niente debug server/verify, Zapper sempre on.
- `zapper_touch.{h,c}` — mouse → touch frontale (tocco = mira + sparo).
- `vita_input.{h,c}` — poll `SDL_FINGER*` + mappatura pad.
- `coroutine_vita.c` — rimpiazzo `ucontext.h` (sincrono, niente stack privato).
- `vita_main.c` — `main()` Vita (path `ux0:data/DUCKHUNT1/`, niente picker/GUI).
- `stub_generated.c` — stub CI senza ROM.
- `package.sh` — `vita-mksfoex` + `vita-make-fself` + zip.

Licenza del gioco ricompilato: vedi repo upstream (PolyForm Noncommercial).
Questo port non include nessuna ROM.
