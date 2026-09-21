#ifndef PRESET_SD_H
#define PRESET_SD_H

#include <SD.h>
#include <Arduino.h>
#include "globals.h"

// =========================================================================
// preset_sd.h — Gestione preset su SD (formato binario K-V)
// =========================================================================
// Struttura:
//   /preset/synthA/
//     ├── preset_00.bin .. preset_29.bin
//     └── nomi_presetA.txt         (30 righe, una per preset)
//   /preset/synthB/
//     ├── preset_00.bin .. preset_29.bin
//     └── nomi_presetB.txt
//
// Ogni file .bin contiene un blob K-V che descrive UNA voce.
// Il Display NON interpreta il contenuto: lo invia al synth così com'è.
// =========================================================================

#define PRESET_PATH_SYNTH_A   "/preset/synthA/"
#define PRESET_PATH_SYNTH_B   "/preset/synthB/"
#define PRESET_FILE_SYNTH_A   "preset_%02d.bin"
#define PRESET_FILE_SYNTH_B   "preset_%02d.bin"
#define PRESET_NAMES_A_FILE   "nomi_presetA.txt"
#define PRESET_NAMES_B_FILE   "nomi_presetB.txt"
#define MAX_PRESET            30
#define PRESET_NAME_LEN       32
#define PRESET_FILENAME_LEN   32
#define PRESET_BIN_MAX_LEN    512

// ========================== EXTERN ==========================
// Nomi dei preset (uno per preset, 30 per synth)
extern char nome_presetA[MAX_PRESET][PRESET_NAME_LEN];
extern char nome_presetB[MAX_PRESET][PRESET_NAME_LEN];

// ========================== PROTOTIPI ==========================
void initPresetNamesA();
void initPresetNamesB();
bool presetFileExists(int synth, int num);                        // synth: 0=A, 1=B
bool readPresetFile(int synth, int num,
                    uint8_t *buf, uint16_t *lenOut);
bool writePresetFile(int synth, int num,
                     const uint8_t *buf, uint16_t len);
bool savePresetNamesToSD(int synth);
bool loadPresetNamesFromSD(int synth);
bool loadAllPresetNames();
void init_sd();

// ========================== FUNZIONI PRIVATE ==========================
static String getPresetPath(int synth, int num) {
    String path = (synth == 0) ? String(PRESET_PATH_SYNTH_A)
                               : String(PRESET_PATH_SYNTH_B);
    char fname[PRESET_FILENAME_LEN];
    snprintf(fname, sizeof(fname), "preset_%02d.bin", num);
    return path + fname;
}

static String getNamesPath(int synth) {
    return (synth == 0)
        ? String(PRESET_PATH_SYNTH_A) + PRESET_NAMES_A_FILE
        : String(PRESET_PATH_SYNTH_B) + PRESET_NAMES_B_FILE;
}

static void ensureDirectory(const char* path) {
    if (!SD.exists(path)) {
        if (!SD.mkdir(path)) {
            Serial.print("mkdir failed: ");
            Serial.println(path);
        }
    }
}

#endif