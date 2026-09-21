#pragma once

// =========================================================================
// preset_transfer.h — Trasferimento preset da SD al SynthA / SynthB via LWS
// =========================================================================
// Va incluso DOPO comunicazioni.h nel .ino.
//
// Dipende da:
//   - Serial1 (bus LWS verso il Router)
//   - next_seq() e lws_send_frame() (definiti in comunicazioni.h / serial_protocol.h)
//   - leggiSer() (poll LWS dal Router)
//   - com_poll() (retry pending ACK)
//   - SD (già montata)
//   - log_add() (log su UI)
// =========================================================================

// -------------------------------------------------------------------------
// Attesa ACK con timeout
// -------------------------------------------------------------------------
static bool waitPresetAck(uint32_t timeoutMs) {
    preset_ack_received = false;
    preset_ack_status   = 0;
    uint32_t t0 = millis();
    while (millis() - t0 < timeoutMs) {
        leggiSer();       // poll LWS dal Router
        com_poll();       // retry pending (utile se in futuro useremo REL)
        if (preset_ack_received) return (preset_ack_status == 0);
        delay(1);
    }
    return false;
}
// Carica il preset N su tutte le voci del SynthA (Poly mode)
// -------------------------------------------------------------------------
// Invia un preset da SD verso un target/voce
//
// Nome file cercato: /presets/<target><id:02>.bin
//   Es. /presets/a00.bin, /presets/b03.bin, /presets/B12.bin
//
// Ritorna true se ACK OK.
// -------------------------------------------------------------------------
// Sostituisci la parte "nome file" in sendPresetToVoice

// Carica il preset N su tutte le voci del SynthA (modalità Poly)
bool loadPresetPoly(uint8_t presetId) {
    static const struct { char tgt; uint8_t voice; } voci[] = {
        {'a', 0}, {'b', 1}, {'b', 2}, {'c', 3}, {'c', 4}
    };
    for (size_t i = 0; i < sizeof(voci)/sizeof(voci[0]); i++) {
        if (!sendPresetToVoice(voci[i].tgt, voci[i].voice, presetId))
            return false;
    }
    return true;
}

// Carica preset diversi per voce (modalità MultiMono)
// `map[5]` = preset da caricare per ciascuna voce
bool loadPresetMultiMono(const uint8_t map[5]) {
    static const struct { char tgt; uint8_t voice; } voci[] = {
        {'a', 0}, {'b', 1}, {'b', 2}, {'c', 3}, {'c', 4}
    };
    for (size_t i = 0; i < 5; i++) {
        if (!sendPresetToVoice(voci[i].tgt, voci[i].voice, map[i]))
            return false;
    }
    return true;
}

static bool sendPresetToVoice(char target, uint8_t voice, uint8_t presetId) {
    // Determina synth
    int synth;
    if (target == 'a' || target == 'b' || target == 'c') synth = 0;   // SynthA
    else if (target == 'B')                              synth = 1;   // SynthB
    else return false;

    if (presetId >= MAX_PRESET) return false;

    uint8_t buf[PRESET_BIN_MAX_LEN];
    uint16_t len;
    if (!readPresetFile(synth, presetId, buf, &len)) {
        char msg[48];
        snprintf(msg, sizeof(msg), "Preset %c%02u mancante", target, presetId);
        log_add(msg, lv_color_hex(0xFF0000));
        return false;
    }

    uint8_t crc = lws_crc8(buf, len);

    // BEGIN
    uint8_t pb[5] = { (uint8_t)target, voice, presetId,
                      (uint8_t)(len & 0xFF), (uint8_t)(len >> 8) };
    lws_send_frame(Serial1, ID_DISPLAY, next_seq(),
                   CMD_PRESET_BEGIN, pb, 5);
    if (!waitPresetAck(500)) { log_add("BEGIN no ACK", lv_color_hex(0xFF0000)); return false; }

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
    lws_send_frame(Serial1, ID_DISPLAY, next_seq(),
                   CMD_PRESET_END, eb, 3);
    if (!waitPresetAck(1000)) { log_add("END no ACK", lv_color_hex(0xFF0000)); return false; }

    char msg[48];
    snprintf(msg, sizeof(msg), "Preset %c%02u OK", target, presetId);
    log_add(msg, lv_color_hex(0x00FF00));
    return true;
}