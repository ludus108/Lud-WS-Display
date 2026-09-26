#pragma once
#include "preset_sd.h"
#include "preset_cache.h"
#include "../../comunicazioni.h"
#include "preset_ui.h"

// =========================================================================
// preset_transfer.h — Load / save dei preset fra SD e synth
// =========================================================================

// -------------------------------------------------------------------------
// Attesa ACK con timeout (già presente)
// -------------------------------------------------------------------------
static bool waitPresetAck(uint32_t timeoutMs) {
    preset_ack_received = false;
    preset_ack_status   = 0;
    uint32_t t0 = millis();
    while (millis() - t0 < timeoutMs) {
        leggiSer();
        com_poll();
        if (preset_ack_received) return (preset_ack_status == 0);
        delay(1);
    }
    return false;
}

// -------------------------------------------------------------------------
// Invia un blob già in RAM al synth (basso livello, no SD)
// Ritorna true se ACK OK.
// -------------------------------------------------------------------------
static bool sendBlobToVoice(char target, uint8_t voice,
                            const uint8_t *buf, uint16_t len) {
    if (len == 0 || len > PRESET_BIN_MAX_LEN) return false;

    uint8_t crc = lws_crc8(buf, len);

    // BEGIN
    uint8_t pb[5] = { (uint8_t)target, voice, 0,
                      (uint8_t)(len & 0xFF), (uint8_t)(len >> 8) };
    lws_send_frame(Serial1, ID_DISPLAY, next_seq(), CMD_PRESET_BEGIN, pb, 5);
    if (!waitPresetAck(500)) return false;

    // CHUNK
    uint16_t off = 0;
    while (off < len) {
        uint8_t chunk = (uint8_t)min((uint16_t)64, (uint16_t)(len - off));
        uint8_t cb[4 + 64];
        cb[0] = (uint8_t)target;
        cb[1] = voice;
        cb[2] = (uint8_t)(off & 0xFF);
        cb[3] = (uint8_t)(off >> 8);
        memcpy(&cb[4], &buf[off], chunk);
        lws_send_frame(Serial1, ID_DISPLAY, next_seq(),
                       CMD_PRESET_CHUNK, cb, 4 + chunk);
        off += chunk;
        delayMicroseconds(500);
    }

    // END
    uint8_t eb[3] = { (uint8_t)target, voice, crc };
    lws_send_frame(Serial1, ID_DISPLAY, next_seq(), CMD_PRESET_END, eb, 3);
    return waitPresetAck(1000);
}

// -------------------------------------------------------------------------
// LOAD da SD: legge, popola la cache, invia
// -------------------------------------------------------------------------
static bool loadPresetToVoice(char target, uint8_t voice,
                              uint8_t presetId, PresetCache *cache) {
    int synth = (target == 'B') ? 1 : 0;

    uint8_t buf[PRESET_BIN_MAX_LEN];
    uint16_t len;
    if (!readPresetFile(synth, presetId, buf, &len)) {
        char msg[48];
        snprintf(msg, sizeof(msg), "Preset %c%02u mancante", target, presetId);
        log_add(msg, lv_color_hex(0xFF0000));
        return false;
    }

    // 1) Popola cache (se richiesto)
    if (cache) presetCacheFromBlob(*cache, buf, len);

    // 2) Invia al synth
    if (!sendBlobToVoice(target, voice, buf, len)) {
        log_add("Preset trasferimento FAIL", lv_color_hex(0xFF0000));
        return false;
    }

    char msg[48];
    snprintf(msg, sizeof(msg), "Preset %c%02u OK", target, presetId);
    log_add(msg, lv_color_hex(0x00FF00));
    return true;
}

// -------------------------------------------------------------------------
// LOAD preset SynthA in Poly (stesso preset su tutte le 5 voci)
// -------------------------------------------------------------------------
static bool loadPresetSynthA_Poly(uint8_t presetId) {
    static const struct { char tgt; uint8_t voice; } voci[] = {
        {'a', 0}, {'b', 1}, {'b', 2}, {'c', 3}, {'c', 4}
    };
    for (size_t i = 0; i < 5; i++) {
        PresetCache *cache = &cacheA[voci[i].voice];
        if (!loadPresetToVoice(voci[i].tgt, voci[i].voice, presetId, cache))
            return false;
    }
    return true;
}

// -------------------------------------------------------------------------
// LOAD preset SynthA in MultiMono (mappa per voce)
// -------------------------------------------------------------------------
static bool loadPresetSynthA_MultiMono(const uint8_t map[5]) {
    static const struct { char tgt; uint8_t voice; } voci[] = {
        {'a', 0}, {'b', 1}, {'b', 2}, {'c', 3}, {'c', 4}
    };
    for (size_t i = 0; i < 5; i++) {
        PresetCache *cache = &cacheA[voci[i].voice];
        if (!loadPresetToVoice(voci[i].tgt, voci[i].voice, map[i], cache))
            return false;
    }
    return true;
}

// -------------------------------------------------------------------------
// LOAD preset SynthB
// -------------------------------------------------------------------------
static bool loadPresetSynthB(uint8_t presetId) {
    return loadPresetToVoice('B', 0, presetId, &cacheB);
}

// -------------------------------------------------------------------------
// SAVE: serializza la cache e scrive su SD
// -------------------------------------------------------------------------
static bool savePresetFromCache(int synth, uint8_t presetId,
                                const PresetCache &cache) {
    uint8_t buf[PRESET_BIN_MAX_LEN];
    uint16_t len = presetCacheBuildBlob(cache, buf, sizeof(buf));
    if (len == 0) {
        log_add("Serialize FAIL", lv_color_hex(0xFF0000));
        return false;
    }
    if (!writePresetFile(synth, presetId, buf, len)) {
        log_add("Scrittura SD FAIL", lv_color_hex(0xFF0000));
        return false;
    }
    char msg[48];
    snprintf(msg, sizeof(msg), "Preset %s%02u salvato", synth==0?"A":"B", presetId);
    log_add(msg, lv_color_hex(0x00FF00));
    return true;
}

// -------------------------------------------------------------------------
// SAVE facade
// -------------------------------------------------------------------------
static bool savePresetA(uint8_t voice, uint8_t presetId) {
    if (voice >= CACHE_VOCI_A) return false;
    return savePresetFromCache(0, presetId, cacheA[voice]);
}

static bool savePresetB(uint8_t presetId) {
    return savePresetFromCache(1, presetId, cacheB);
}