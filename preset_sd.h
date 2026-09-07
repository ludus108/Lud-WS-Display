#ifndef PRESET_SD_H
#define PRESET_SD_H

#include <SD.h>
#include <Arduino.h>
#include "globals.h"

// ========================== COSTANTI ==========================
#define PRESET_PATH_SYNTH_A "/preset/synthA/"
#define PRESET_PATH_SYNTH_B "/preset/synthB/"
#define MAX_PRESET 16
#define PRESET_FILENAME_LEN 20

// ========================== DICHIARAZIONI ESTERNE ==========================
extern int presetNumA, presetNumB;
extern int timbrA[MAX_PRESET][MAX_timbrA];
extern int timbrB[MAX_PRESET][MAX_timbrB];
extern char presetNamesA[MAX_PRESET][MAX_timbrA];
extern char presetNamesB[MAX_PRESET][MAX_timbrB];

// ========================== PROTOTIPI FUNZIONI ==========================
void initPresetValues();
void initPresetNamesA();
void initPresetNamesB();
bool savePresetToSD(int synth, int num);
bool loadPresetFromSD(int synth, int num);
bool saveNamesToSD(int synth);
bool loadNamesFromSD(int synth);
bool saveAllToSD();
bool loadAllFromSD();
bool saveCurrentPresetA();
bool saveCurrentPresetB();
bool loadCurrentPresetA();
bool loadCurrentPresetB();
void init_sd();

// ========================== FUNZIONI PRIVATE STATICHE ==========================
static String getPresetPath(int synth, int num) {
    String path = (synth == 0) ? String(PRESET_PATH_SYNTH_A) : String(PRESET_PATH_SYNTH_B);
    char fname[PRESET_FILENAME_LEN];
    snprintf(fname, sizeof(fname), "preset_%02d.csv", num);
    return path + fname;
}

static void ensureDirectory(const char* path) {
    if (!SD.exists(path)) {
        if (!SD.mkdir(path)) {
            Serial.print("Failed to create directory: ");
            Serial.println(path);
        }
    }
}

#endif