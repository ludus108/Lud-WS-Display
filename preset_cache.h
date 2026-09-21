#ifndef PRESET_CACHE_H
#define PRESET_CACHE_H

#include <Arduino.h>
#include "preset_sd.h"   // MAX_PRESET

// =========================================================================
// preset_cache.h — Cache locale dei parametri inviati ai synth
// =========================================================================
// Il Display tiene per ogni voce di SynthA e per SynthB una cache dei
// valori correnti. Quando l'utente modifica un parametro, la cache si
// aggiorna e il comando viene inviato via LWS.
//
// Il salvataggio serializza la cache in formato K-V binario (identico
// a quello atteso dai synth).
// =========================================================================

// Tipi di parametro (coerenti con la serializzazione K-V)
#define PTYPE_UNSET   0
#define PTYPE_U8      1     // 1 byte
#define PTYPE_I16     2     // 2 byte
#define PTYPE_I32     4     // 4 byte

// Numero di voci di SynthA
#define CACHE_VOCI_A  5

// Struttura di una singola voce (o di SynthB)
struct PresetCache {
    int32_t  value[256];     // valore per ogni possibile key byte
    uint8_t  type[256];      // PTYPE_*  (0 = non settato)
};

// ========================== EXTERN ==========================
// 5 voci di SynthA (0..4 = a, b-b, b-c, c-b, c-c)
extern PresetCache cacheA[CACHE_VOCI_A];

// SynthB (unica voce logica)
extern PresetCache cacheB;

// ========================== API ==========================
void presetCacheClear(PresetCache &c);                      // azzera tutti i campi
void presetCacheClearAll();                                  // azzera tutti (A + B)
void presetCacheSet(PresetCache &c, char key, int32_t v, uint8_t t);
int32_t presetCacheGet(const PresetCache &c, char key);
bool presetCacheHas(const PresetCache &c, char key);

// Serializza la cache in un blob K-V + CRC
// ritorna la lunghezza in byte, o 0 se errore/overflow
uint16_t presetCacheBuildBlob(const PresetCache &c,
                              uint8_t *buf, uint16_t maxLen);

// Popola la cache leggendo un blob K-V
// ritorna true se il parse è ok
bool presetCacheFromBlob(PresetCache &c,
                         const uint8_t *buf, uint16_t len);

#endif