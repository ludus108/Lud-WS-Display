#include "preset_cache.h"
#include "../../serial_protocol.h"   // per lws_crc8

// ========================== DEFINIZIONI ==========================
PresetCache cacheA[CACHE_VOCI_A];
PresetCache cacheB;

// ========================== CLEAR ==========================
void presetCacheClear(PresetCache &c) {
    memset(c.value, 0, sizeof(c.value));
    memset(c.type,  0, sizeof(c.type));
}

void presetCacheClearAll() {
    for (int i = 0; i < CACHE_VOCI_A; i++) presetCacheClear(cacheA[i]);
    presetCacheClear(cacheB);
}

// ========================== SET / GET ==========================
void presetCacheSet(PresetCache &c, char key, int32_t v, uint8_t t) {
    uint8_t k = (uint8_t)key;
    c.value[k] = v;
    c.type[k]  = t;
}

int32_t presetCacheGet(const PresetCache &c, char key) {
    return c.value[(uint8_t)key];
}

bool presetCacheHas(const PresetCache &c, char key) {
    return c.type[(uint8_t)key] != PTYPE_UNSET;
}

// ========================== SERIALIZZAZIONE ==========================
// Formato:
//   [version=0x01]
//   [key][len][value...]   per ogni entry settata (type != 0)
//   [0xFF]                 terminatore
//   [crc8]
// ==========================
uint16_t presetCacheBuildBlob(const PresetCache &c,
                              uint8_t *buf, uint16_t maxLen) {
    if (maxLen < 4) return 0;

    uint16_t pos = 0;
    buf[pos++] = 0x01;   // version

    for (int k = 0; k < 256; k++) {
        if (c.type[k] == PTYPE_UNSET) continue;

        uint8_t len = c.type[k];
        if (pos + 2 + len + 2 > maxLen) {
            // overflow: aggiungo terminatore e CRC dove possibile
            if (pos + 2 > maxLen) return 0;
            buf[pos++] = 0xFF;
            uint8_t crc = lws_crc8(buf, pos);
            buf[pos++] = crc;
            return pos;
        }

        buf[pos++] = (uint8_t)k;
        buf[pos++] = len;
        int32_t v = c.value[k];

        for (int i = 0; i < len; i++) {
            buf[pos++] = (uint8_t)((v >> (8 * i)) & 0xFF);
        }
    }

    if (pos + 2 > maxLen) return 0;
    buf[pos++] = 0xFF;   // terminatore
    uint8_t crc = lws_crc8(buf, pos);
    buf[pos++] = crc;
    return pos;
}

// ========================== DESERIALIZZAZIONE ==========================
bool presetCacheFromBlob(PresetCache &c,
                         const uint8_t *buf, uint16_t len) {
    if (len < 2) return false;

    presetCacheClear(c);

    uint16_t pos = 0;
    uint8_t version = buf[pos++];
    if (version != 0x01) return false;

    while (pos < len) {
        uint8_t key = buf[pos++];
        if (key == 0xFF) break;

        if (pos >= len) return false;
        uint8_t entryLen = buf[pos++];

        if (pos + entryLen > len) return false;

        // Ricostruisci valore signed/unsigned
        int32_t v = 0;
        for (uint8_t i = 0; i < entryLen && i < 4; i++) {
            v |= ((int32_t)buf[pos + i]) << (8 * i);
        }
        // sign-extend se il valore è signed e il MSB è 1
        // (non necessario per ora: i parametri signed sono -24..+24 e cablati
        //  nel parser del synth. Qui la cache li memorizza grezzi.)

        presetCacheSet(c, (char)key, v, entryLen);
        pos += entryLen;
    }
    return true;
}