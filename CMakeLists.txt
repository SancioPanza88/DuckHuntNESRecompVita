cmake_minimum_required(VERSION 3.20)
project(DuckHuntRecompVita C)

set(CMAKE_C_STANDARD 11)

# ---------------------------------------------------------------------------
# Duck Hunt NES Recomp — port PS Vita (VitaSDK)
# Build PC (dev):   cmake -S . -B build && cmake --build build
# Build Vita:       cmake -S . -B build-vita \
#                     -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake \
#                     -DCMAKE_BUILD_TYPE=Release \
#                   && cmake --build build-vita
# Il .vpk si crea con gli stessi step documentati in README.md / vita/package.sh
# (vita-mksfoex + vita-make-fself + zip).
# ---------------------------------------------------------------------------

set(NESRECOMP_ROOT ${CMAKE_SOURCE_DIR}/nesrecomp)

# Su Vita il TCP debug server non serve e costa RAM: forziamo lo stub no-op.
# (runner.cmake compila debug_server_stub.c invece di debug_server.c)
set(NESRECOMP_ENABLE_TRACE OFF CACHE BOOL "TCP debug server (sempre OFF su Vita)" FORCE)

include(${NESRECOMP_ROOT}/runner/runner.cmake)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# --- Sorgenti generati dal recompiler (ROM -> C, fatto su PC host) -----------
# La CI non ha la ROM (copyright): se generated/ manca, usa lo stub e la build
# resta verde (prova il toolchain). Con la ROM vera, rigenera su PC e ricompila.
if(EXISTS "${CMAKE_SOURCE_DIR}/generated/duck-hunt_full.c"
   AND EXISTS "${CMAKE_SOURCE_DIR}/generated/duck-hunt_dispatch.c")
  set(GENERATED_SRC
    ${CMAKE_SOURCE_DIR}/generated/duck-hunt_full.c
    ${CMAKE_SOURCE_DIR}/generated/duck-hunt_dispatch.c)
  message(STATUS "Uso codice generato da ROM reale in generated/")
else()
  set(GENERATED_SRC ${CMAKE_SOURCE_DIR}/vita/stub_generated.c)
  message(STATUS "generated/ assente (nessuna ROM in CI) — uso vita/stub_generated.c")
endif()

# --- Rimuoviamo i file POSIX/PC non compilabili su Vita ----------------------
# coroutine.c usa ucontext.h (makecontext/swapcontext): newlib di VitaSDK non
# lo supporta su ARM (manca lo switch di stack privato). Sostituito da
# vita/coroutine_vita.c (backend sincrono, senza stack separato — sufficiente
# per Duck Hunt NROM che non usa scheduler cooperativi).
# launcher.c contiene main() PC (file-picker Win32/Cocoa/console): su Vita
# usiamo vita/vita_main.c (main con path fissi ux0:data/... + sceIoChdir).
set(VITA_RUNNER_SOURCES ${NESRECOMP_RUNNER_SOURCES})
list(REMOVE_ITEM VITA_RUNNER_SOURCES
  ${NESRECOMP_ROOT}/runner/src/coroutine.c
  ${NESRECOMP_ROOT}/runner/src/launcher.c)

if(DEFINED ENV{VITASDK})
  set(BUILD_VITA TRUE)
endif()

if(BUILD_VITA)
  add_executable(DuckHuntRecompVita
    ${VITA_RUNNER_SOURCES}
    ${GENERATED_SRC}
    vita/extras_vita.c
    vita/coroutine_vita.c
    vita/zapper_touch.c
    vita/vita_input.c
    vita/vita_compat.c
    vita/vita_main.c)
else()
  # Build PC di sviluppo: riusa extras.c originale + launcher/coroutine upstream
  add_executable(DuckHuntRecompVita
    ${NESRECOMP_RUNNER_SOURCES}
    ${GENERATED_SRC}
    vita/extras_vita.c
    vita/coroutine_vita.c
    vita/zapper_touch.c
    vita/vita_input.c
    vita/vita_compat.c
    vita/vita_main.c)
endif()

target_include_directories(DuckHuntRecompVita PRIVATE
  ${NESRECOMP_RUNNER_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/vita
  ${SDL2_INCLUDE_DIRS})

target_link_libraries(DuckHuntRecompVita PRIVATE ${SDL2_LIBRARIES})

target_compile_definitions(DuckHuntRecompVita PRIVATE
  RECOMP_STACK_TRACKING
  NESRECOMP_GAME_ZAPPER)

if(BUILD_VITA)
  # --- Flag VitaSDK ---------------------------------------------------------
  target_compile_definitions(DuckHuntRecompVita PRIVATE __VITA__=1 VITA=1)
  # newlib ARM: niente ucontext, niente eccezioni C++ (puro C comunque)
  target_compile_options(DuckHuntRecompVita PRIVATE -O2 -fno-strict-aliasing)
  target_link_libraries(DuckHuntRecompVita PRIVATE
    SceDisplay_stub SceCtrl_stub SceTouch_stub SceAudio_stub
    SceGxm_stub SceCommonDialog_stub ScePower_stub SceHid_stub
    SceMotion_stub SceAppMgr_stub SceIofilemgr_stub
    pthread
    m c)

  include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

  set(VITA_TITLEID "DUCKHUNT1")
  set(VITA_TITLE "Duck Hunt Recomp")

  vita_create_self(eboot.bin DuckHuntRecompVita UNSAFE)
  vita_create_vpk(DuckHuntRecompVita.vpk ${VITA_TITLEID} eboot.bin
    VERSION "01.00"
    NAME "${VITA_TITLE}")
endif()

if(MSVC)
  target_compile_options(DuckHuntRecompVita PRIVATE /W3 /WX-)
  target_compile_options(DuckHuntRecompVita PRIVATE /wd4102)
endif()

if(NOT BUILD_VITA AND NOT MSVC)
  target_compile_options(DuckHuntRecompVita PRIVATE -Wno-implicit-function-declaration -Wno-implicit-int)
endif()
