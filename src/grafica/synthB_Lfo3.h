#pragma once
#include "synthB_Config.h"
#include "synthB_State.h"

// =========================================================================
// synthB_Lfo3.h — Terzo LFO con 3 contatori indipendenti e spread
// =========================================================================
// Genera 8 forme d'onda selezionabili e le invia a 3 VCF esterni.
// I 3 counter condividono la forma ma sono sfasati da `lfo3Spread`:
//   spread = 0    → tutti in fase (0°)
//   spread = 255  → counter 0 a 0°, counter 1 a 90°, counter 2 a 180°
// =========================================================================

// --- Forme d'onda ---
#define LFO3_SINE    0
#define LFO3_SAW     1
#define LFO3_RSAW    2
#define LFO3_SQR     3
#define LFO3_RND     4
#define LFO3_RND50   5
#define LFO3_RND25   6
#define LFO3_RND10   7

// --- Stato (statico al file) ---
static uint8_t  lfo3Wave     = LFO3_SINE;
static uint8_t  lfo3Spread   = 0;         // 0..255
static int32_t  lfo3Rate     = 12000;     // µs (int32)
static int32_t  lfo3Lev      = 0;         // 0..1023
static uint32_t lfo3LastTick = 0;

static uint8_t  lfo3BasePhase = 0;        // counter 0..255 comune
static int      lfo3RndHold[3] = {128, 128, 128};   // hold indipendenti per RND

// --- Tabelle forme d'onda ---
static int lfo3SineArr[256];
static int lfo3SawArr[256];
static int lfo3RsawArr[256];

// --- Output verso VCF (0..255 ciascuno) ---
static uint8_t  vcf1Out = 0;
static uint8_t  vcf2Out = 0;
static uint8_t  vcf3Out = 0;

// --- Valori base dei cutoff (impostati da K_VCF*_CUT) ---
static uint8_t  vcf1Base = 128;
static uint8_t  vcf2Base = 128;
static uint8_t  vcf3Base = 128;

// -------------------------------------------------------------------------
// INIT
// -------------------------------------------------------------------------
static void initLfo3() {
    for (int i = 0; i < 256; i++) {
        // SINE: -511..+511
        lfo3SineArr[i] = (int)(sinf(PIx2 * i / 256.0f) * 511.0f);
        // SAW: -511..+511 (rampa in salita)
        lfo3SawArr[i] = ((i * 1022) / 255) - 511;
        // RSAW: +511..-511 (rampa in discesa)
        lfo3RsawArr[i] = 511 - ((i * 1022) / 255);
    }
    lfo3LastTick = micros();
    LWS_DEBUG.println("[B] LFO3 init OK");
}

// -------------------------------------------------------------------------
// Ritorna un campione della forma selezionata per il counter `c` (0..2)
// Range output: -511..+511
// -------------------------------------------------------------------------
static int lfo3Sample(uint8_t phase, uint8_t c) {
    switch (lfo3Wave) {
        case LFO3_SINE:  return lfo3SineArr[phase];
        case LFO3_SAW:   return lfo3SawArr[phase];
        case LFO3_RSAW:  return lfo3RsawArr[phase];
        case LFO3_SQR:   return (phase < 128) ? 511 : -511;

        // RND: le 3 uscite sono indipendenti (rndHold[c])
        case LFO3_RND:   return lfo3RndHold[c] - 128;
        case LFO3_RND50: return lfo3RndHold[c] - 128;
        case LFO3_RND25: return lfo3RndHold[c] - 128;
        case LFO3_RND10: return lfo3RndHold[c] - 128;

        default: return 0;
    }
}

// -------------------------------------------------------------------------
// Aggiorna un rndHold con probabilità data (solo per forme RND*)
// -------------------------------------------------------------------------
static inline void lfo3UpdateRnd(uint8_t c, uint8_t percent) {
    if ((uint8_t)random(100) < percent) {
        lfo3RndHold[c] = (int)random(256);
    }
}

// -------------------------------------------------------------------------
// TICK — chiamato da loop() ad ogni iterazione
// -------------------------------------------------------------------------
static void lfo3Tick() {
    uint32_t now = micros();
    if ((now - lfo3LastTick) < (uint32_t)lfo3Rate) return;
    lfo3LastTick = now;

    // Avanza il counter base
    lfo3BasePhase++;
    if (lfo3BasePhase > 255) lfo3BasePhase = 0;

    // Calcola le fasi sfasate
    // offset_counter1 = spread/2, offset_counter2 = spread
    uint8_t phase0 = lfo3BasePhase;
    uint8_t phase1 = (uint8_t)(lfo3BasePhase + (lfo3Spread >> 1));
    uint8_t phase2 = (uint8_t)(lfo3BasePhase + lfo3Spread);

    // Aggiorna i 3 hold random (indipendenti, ognuno al proprio wrap)
    if (lfo3Wave >= LFO3_RND) {
        if (phase0 == 0) {
            switch (lfo3Wave) {
                case LFO3_RND:   lfo3UpdateRnd(0, 100); break;
                case LFO3_RND50: lfo3UpdateRnd(0,  50); break;
                case LFO3_RND25: lfo3UpdateRnd(0,  25); break;
                case LFO3_RND10: lfo3UpdateRnd(0,  10); break;
            }
        }
        if (phase1 == 0) {
            switch (lfo3Wave) {
                case LFO3_RND:   lfo3UpdateRnd(1, 100); break;
                case LFO3_RND50: lfo3UpdateRnd(1,  50); break;
                case LFO3_RND25: lfo3UpdateRnd(1,  25); break;
                case LFO3_RND10: lfo3UpdateRnd(1,  10); break;
            }
        }
        if (phase2 == 0) {
            switch (lfo3Wave) {
                case LFO3_RND:   lfo3UpdateRnd(2, 100); break;
                case LFO3_RND50: lfo3UpdateRnd(2,  50); break;
                case LFO3_RND25: lfo3UpdateRnd(2,  25); break;
                case LFO3_RND10: lfo3UpdateRnd(2,  10); break;
            }
        }
    }

    // Leggi i 3 campioni e applica la depth
    int s0 = lfo3Sample(phase0, 0);
    int s1 = lfo3Sample(phase1, 1);
    int s2 = lfo3Sample(phase2, 2);

    // mod 0..1023 → -511..+511 * depth / 1023
    int mod0 = (s0 * lfo3Lev) / 1023;
    int mod1 = (s1 * lfo3Lev) / 1023;
    int mod2 = (s2 * lfo3Lev) / 1023;

    // Somma al base e clamp 0..255
    int r0 = (int)vcf1Base + mod0 / 2;   // /2 per ridurre ampiezza su 0..255
    int r1 = (int)vcf2Base + mod1 / 2;
    int r2 = (int)vcf3Base + mod2 / 2;

    vcf1Out = (uint8_t)constrain(r0, 0, 255);
    vcf2Out = (uint8_t)constrain(r1, 0, 255);
    vcf3Out = (uint8_t)constrain(r2, 0, 255);

    // TODO: quando il TLC5628 sarà integrato, scrivere vcf1Out, vcf2Out,
    // vcf3Out sui canali 1, 2, 3 del DAC.
    // Per ora il valore è disponibile per debug.
}

// -------------------------------------------------------------------------
// ACCESSOR per debug (opzionale)
// -------------------------------------------------------------------------
static inline void lfo3Dump() {
    LWS_DEBUG.printf("[B] LFO3 wave=%u spread=%u rate=%ld lev=%ld "
                     "out=%u,%u,%u\n",
                     lfo3Wave, lfo3Spread, (long)lfo3Rate, (long)lfo3Lev,
                     vcf1Out, vcf2Out, vcf3Out);
}

