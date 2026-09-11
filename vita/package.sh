#!/bin/sh
# package.sh — packaging manuale in .vpk (gli stessi passaggi della CI/CMake).
# Eseguire DOPO "cmake --build build-vita" dentro il container VitaSDK o con
# VitaSDK installato (VITASDK e PATH già impostati).
# Uso:  sh vita/package.sh [build-dir]   (default: build-vita)
set -eu

BUILD_DIR="${1:-build-vita}"
TITLE_ID="DUCKHUNT1"
TITLE="Duck Hunt Recomp"
VERSION="01.00"

ELF="$BUILD_DIR/DuckHuntRecompVita"
EBOOT="$BUILD_DIR/eboot.bin"
SFO="$BUILD_DIR/param.sfo"
VPK="$BUILD_DIR/DuckHuntRecompVita.vpk"

echo "[1/4] ELF -> $ELF (già prodotto da cmake --build)"
test -f "$ELF" || { echo "Manca $ELF: prima esegui cmake --build $BUILD_DIR"; exit 1; }

echo "[2/4] vita-mksfoex: param.sfo (titleid=$TITLE_ID)"
vita-mksfoex -s "$TITLE_ID" "$TITLE" "$SFO"

echo "[3/4] vita-make-fself: eboot.bin (SELF firmato, UNSAFE per homebrew)"
vita-make-fself -s "$ELF" "$EBOOT"

echo "[4/4] zip -> $VPK (eboot.bin + param.sfo)"
rm -f "$VPK"
(cd "$BUILD_DIR" && zip -j "DuckHuntRecompVita.vpk" eboot.bin param.sfo)

echo "OK: $VPK"
ls -la "$VPK"
