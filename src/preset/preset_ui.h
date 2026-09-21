#ifndef PRESET_UI_H
#define PRESET_UI_H

#include "preset_cache.h"
#include "../../comunicazioni.h"   // send_param_voce, send_param_i32_voce, targetForVoice

// =========================================================================
// preset_ui.h — Helper per l'UI: aggiorna cache + invia LWS
// =========================================================================

// --- Mappa voce globale → target (a/b/c) ---
// Già definito in comunicazioni.h come targetForVoice()
// Lo riuso.

// -------------------------------------------------------------------------
// Imposta un parametro uint8 su una voce del SynthA
// -------------------------------------------------------------------------
static inline void uiSetParamU8(int voice, char key, uint8_t value) {
    if (voice < 0 || voice >= CACHE_VOCI_A) return;

    presetCacheSet(cacheA[voice], key, value, PTYPE_U8);
    send_param_voce(targetForVoice(voice), (uint8_t)voice, key, value);
}

// -------------------------------------------------------------------------
// Imposta un parametro int16/int32 su una voce del SynthA
// -------------------------------------------------------------------------
static inline void uiSetParamI32(int voice, char key, int32_t value) {
    if (voice < 0 || voice >= CACHE_VOCI_A) return;

    presetCacheSet(cacheA[voice], key, value, PTYPE_I32);
    send_param_i32_voce(targetForVoice(voice), (uint8_t)voice, key, value);
}

// -------------------------------------------------------------------------
// Stesso per SynthB (voce logica unica)
// -------------------------------------------------------------------------
static inline void uiSetParamB_U8(char key, uint8_t value) {
    presetCacheSet(cacheB, key, value, PTYPE_U8);
    send_param_voce('B', 0, key, value);
}

static inline void uiSetParamB_I32(char key, int32_t value) {
    presetCacheSet(cacheB, key, value, PTYPE_I32);
    send_param_i32_voce('B', 0, key, value);
}

// -------------------------------------------------------------------------
// Invia un parametro MIDI (nota, bend, CC) — non tocca la cache
// -------------------------------------------------------------------------
static inline void uiSendNote(int voice, uint8_t onoff,
                              uint8_t pitch, uint8_t vel) {
    send_midi_note_v(targetForVoice(voice), (uint8_t)voice,
                     onoff, pitch, vel);
}

#endif