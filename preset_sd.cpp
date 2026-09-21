#include "preset_sd.h"

// ========================== DEFINIZIONI ==========================
char nome_presetA[MAX_PRESET][PRESET_NAME_LEN];
char nome_presetB[MAX_PRESET][PRESET_NAME_LEN];

// ========================== FILE BINARI ==========================
bool presetFileExists(int synth, int num) {
    if (num < 0 || num >= MAX_PRESET) return false;
    String p = getPresetPath(synth, num);
    return SD.exists(p.c_str());
}

bool readPresetFile(int synth, int num,
                    uint8_t *buf, uint16_t *lenOut) {
    if (num < 0 || num >= MAX_PRESET) return false;
    String p = getPresetPath(synth, num);
    File f = SD.open(p.c_str(), FILE_READ);
    if (!f) return false;

    size_t sz = f.size();
    if (sz == 0 || sz > PRESET_BIN_MAX_LEN) { f.close(); return false; }

    size_t r = f.read(buf, sz);
    f.close();
    if (r != sz) return false;

    *lenOut = (uint16_t)sz;
    return true;
}

bool writePresetFile(int synth, int num,
                     const uint8_t *buf, uint16_t len) {
    if (num < 0 || num >= MAX_PRESET) return false;
    if (len == 0 || len > PRESET_BIN_MAX_LEN) return false;

    ensureDirectory((synth == 0) ? PRESET_PATH_SYNTH_A
                                 : PRESET_PATH_SYNTH_B);

    String p = getPresetPath(synth, num);
    SD.remove(p.c_str());

    File f = SD.open(p.c_str(), FILE_WRITE);
    if (!f) return false;

    size_t w = f.write(buf, len);
    f.close();
    return (w == len);
}

// ========================== NOMI ==========================
bool loadPresetNamesFromSD(int synth) {
    String p = getNamesPath(synth);
    File f = SD.open(p.c_str(), FILE_READ);
    if (!f) return false;

    char (*dest)[PRESET_NAME_LEN] = (synth == 0) ? nome_presetA
                                                 : nome_presetB;

    int i = 0;
    while (f.available() && i < MAX_PRESET) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0 || line.length() >= PRESET_NAME_LEN) {
            snprintf(dest[i], PRESET_NAME_LEN, "Preset %s%02d",
                     (synth == 0) ? "A" : "B", i);
        } else {
            snprintf(dest[i], PRESET_NAME_LEN, "%s", line.c_str());
        }
        i++;
    }
    f.close();

    // Completa con default quelli mancanti
    for (; i < MAX_PRESET; i++) {
        snprintf(dest[i], PRESET_NAME_LEN, "Preset %s%02d",
                 (synth == 0) ? "A" : "B", i);
    }
    return true;
}

bool savePresetNamesToSD(int synth) {
    String p = getNamesPath(synth);
    SD.remove(p.c_str());

    File f = SD.open(p.c_str(), FILE_WRITE);
    if (!f) return false;

    char (*src)[PRESET_NAME_LEN] = (synth == 0) ? nome_presetA
                                                : nome_presetB;

    for (int i = 0; i < MAX_PRESET; i++) {
        f.println(src[i]);
    }
    f.close();
    return true;
}

void initPresetNamesA() {
    for (int i = 0; i < MAX_PRESET; i++)
        snprintf(nome_presetA[i], PRESET_NAME_LEN, "Preset A%02d", i);
}

void initPresetNamesB() {
    for (int i = 0; i < MAX_PRESET; i++)
        snprintf(nome_presetB[i], PRESET_NAME_LEN, "Preset B%02d", i);
}

bool loadAllPresetNames() {
    bool okA = loadPresetNamesFromSD(0);
    if (!okA) { initPresetNamesA(); savePresetNamesToSD(0); }

    bool okB = loadPresetNamesFromSD(1);
    if (!okB) { initPresetNamesB(); savePresetNamesToSD(1); }

    return okA && okB;
}

// ========================== INIZIALIZZAZIONE ==========================
void init_sd() {
    ensureDirectory("/preset");
    ensureDirectory(PRESET_PATH_SYNTH_A);
    ensureDirectory(PRESET_PATH_SYNTH_B);
    loadAllPresetNames();
}