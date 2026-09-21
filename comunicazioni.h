#ifndef COMUNICAZIONI_H
#define COMUNICAZIONI_H

/*
 * ============================================================================
 *  comunicazioni.h — LWSv1.1 (LUD-WS Serial Protocol v1.1)
 * ============================================================================
 *
 *  CHANGELOG rispetto a LWSv1:
 *  ---------------------------------------------------------------------------
 *  [10.1] FIX  : ID_POWER aggiunto a MCU_IDS[] e MCU_NAMES[], MAX_MCU = 10.
 *                Prima era presente solo in DISC_TARGETS[] e non veniva mai
 *                contato come online (report max 8/9).
 *
 *  [10.2] ADD  : CRC-8/ATM (poly 0x07, init 0x00) nel frame.
 *                Copre SENDER..PAYLOAD. Frame con CRC errato vengono scartati
 *                silenziosamente dal parser (auto-risincronizzante).
 *
 *  [10.3] ADD  : SEQ byte + protocollo ibrido di ACK:
 *                  - CMD_PING      -> PONG (affidabile, usato in discovery)
 *                  - CMD_PARAM     -> fire-and-forget (slider/pot ad alta freq)
 *                  - CMD_PARAM_REL -> ACK obbligatorio (preset, config, critici)
 *                Coda pending da 8 slot, timeout 300 ms, 3 retry.
 *
 *  Formato frame LWSv1.1:
 *      [SENDER][SEQ][CMD][LEN][PAYLOAD...][CRC8][&][!]
 *
 *  Traffico tipico:
 *      - slider/pot    : ~10 byte/update (nessun ACK)
 *      - comando critico: ~10 byte TX + ~9 byte ACK
 *      - discovery ping : ~10 byte TX + ~9 byte PONG
 *
 *  Note implementative:
 *      - Il parser è auto-risincronizzante su rumore (torna a ST_SENDER
 *        quando i terminatori non corrispondono).
 *      - I byte '&' (0x26) e '!' (0x21) non devono comparire nel payload:
 *        se servono, vanno codificati applicativamente.
 *      - com_poll() DEVE essere chiamato ad ogni iterazione di loop() per
 *        far girare i retry degli ACK in sospeso.
 * ============================================================================
 */

#include <Arduino.h>
#include "globals.h"
#include "serial_protocol.h"

// ========================== MCU IDENTIFIERS ==========================
#define ID_DISPLAY  'D'
#define ID_SYNTH_A1 'a'
#define ID_SYNTH_A2 'b'
#define ID_SYNTH_A3 'c'
#define ID_SYNTH_B  'B'
#define ID_ROUTER   'R'
#define ID_CTRL     'C'
#define ID_MOD      'M'
#define ID_TEENSY   'T'
#define ID_POWER    'P'

#define MAX_MCU         10          // [10.1] era 9, ora include Power
#define PING_TIMEOUT_MS 2000


// ========================== PARAMETER MAPPING (invariato) ==========================
struct ParamMapA { char key; uint8_t index; };
static const ParamMapA mapA[] = {
    {'a', wave_mode_A},    {'b', wave_A},          {'c', shape_A},         {'d', shape_lev_A},
    {'e', shape_rate_A},   {'f', lfo_pitch_lev_A}, {'g', cutOff_A},        {'h', res_A},
    {'i', vcf_lfo_A},      {'l', vcf_env_A},       {'m', vcf_ana_env_A},   {'n', ana_ATTACK_A},
    {'o', ana_DECAY_A},    {'p', ana_SUSTAIN_A},   {'q', ana_RELEASE_A},   {'r', vir_ATTACK_A},
    {'s', vir_DECAY_A},    {'t', vir_SUSTAIN_A},   {'u', vir_RELEASE_A},   {'v', lfo_wave_A},
    {'z', lfo_rate_A},     {'x', vca_vir_env_A},   {'y', vca_lfo_A}
};
static const uint8_t MAPA_SIZE = sizeof(mapA) / sizeof(mapA[0]);

struct ParamMapB { char key; uint8_t index; };
static const ParamMapB mapB[] = {
    {'a', wave_mode_B},    {'b', wave_B},          {'c', shape_B},         {'d', shape_lev_B},
    {'e', shape_rate_B},   {'f', lfo_pitch_lev_B}, {'g', vcf_mode_B},      {'h', cutOff_1_B},
    {'i', cutOff_2_B},     {'l', cutOff_3_B},      {'m', res_B},           {'n', vcf_lfo_B},
    {'o', vcf_env_B},      {'p', vcf_Bna_env_B},   {'q', ana_BTTACK_B},    {'r', ana_DECAY_B},
    {'s', ana_SUSTAIN_B},  {'t', ana_RELEASE_B},   {'u', vir_BTTACK_B},    {'v', vir_DECAY_B},
    {'z', vir_SUSTAIN_B},  {'x', vir_RELEASE_B},   {'y', lfo_wave_B},      {'w', lfo_rate_B},
    {'k', vca_vir_env_B},  {'j', vca_lfo_B}
};
static const uint8_t MAPB_SIZE = sizeof(mapB) / sizeof(mapB[0]);

// ========================== MCU STATUS ==========================
static struct {
    bool    online[MAX_MCU];
    uint8_t ping_counter;
    bool    all_mcu_ok;
} mcu_status = {};

static const char* MCU_NAMES[MAX_MCU] = {
    "Display", "Synth A1", "Synth A2", "Synth A3",
    "Synth B", "Router",   "Ctrl",     "Mod",
    "Teensy",  "Power"                       // [10.1] aggiunto
};
static const char MCU_IDS[MAX_MCU] = {
    ID_DISPLAY, ID_SYNTH_A1, ID_SYNTH_A2, ID_SYNTH_A3,
    ID_SYNTH_B, ID_ROUTER,   ID_CTRL,     ID_MOD,
    ID_TEENSY,  ID_POWER                     // [10.1] aggiunto
};

static LwsParser lwsDisplayParser;
static inline char targetForVoice(uint8_t voice) {
    if (voice == 0)                return 'a';
    if (voice == 1 || voice == 2)  return 'b';
    if (voice == 3 || voice == 4)  return 'c';
    return 'a';
}
// ========================== SEQ + PENDING ACK ==========================
static uint8_t g_tx_seq = 0;
static inline uint8_t next_seq() { return g_tx_seq++; }

#define PENDING_MAX     8
#define ACK_TIMEOUT_MS  300
#define ACK_RETRIES_MAX 3

struct PendingAck {
    bool     used;
    uint8_t  seq;
    char     target;
    uint8_t  cmd;
    uint8_t  len;
    uint8_t  data[8];
    uint8_t  retries;
    uint32_t t_sent;
};
static PendingAck g_pending[PENDING_MAX] = {};

static PendingAck* pending_find(uint8_t seq) {
    for (int i = 0; i < PENDING_MAX; i++)
        if (g_pending[i].used && g_pending[i].seq == seq) return &g_pending[i];
    return nullptr;
}
static PendingAck* pending_alloc() {
    for (int i = 0; i < PENDING_MAX; i++)
        if (!g_pending[i].used) return &g_pending[i];
    return nullptr;
}

// ========================== TX ==========================
static void send_ping(char target_mcu) {
    uint8_t p[1] = { (uint8_t)target_mcu };
    lws_send_frame(Serial1, ID_DISPLAY, next_seq(), CMD_PING, p, 1);
    for (int i = 0; i < MAX_MCU; i++) {
        if (MCU_IDS[i] == target_mcu) {
            Serial.print("Ping -> "); Serial.println(MCU_NAMES[i]);
            return;
        }
    }
}

static void send_pong() {
    lws_send_frame(Serial1, ID_DISPLAY, next_seq(), CMD_PONG, nullptr, 0);
}

// -------- Fire-and-forget: per slider/pot in movimento, alta frequenza --------
// Non entra in coda pending, nessun retry. Il ricevente aggiorna e basta.
// Se un frame si perde, il successivo (33 ms dopo) lo sovrascrive.
static void send_param_update(char target, char param_key, uint8_t value) {
    uint8_t p[3] = { (uint8_t)target, (uint8_t)param_key, value };
    lws_send_frame(Serial1, ID_DISPLAY, next_seq(), CMD_PARAM, p, 3);
}

// -------- Reliable: per preset load, config, cambi wave mode, ecc. --------
// Entra in coda pending, retry fino a 3 volte con timeout 300 ms.
static void send_param_reliable(char target, char param_key, uint8_t value) {
    uint8_t p[3] = { (uint8_t)target, (uint8_t)param_key, value };
    uint8_t seq  = next_seq();

    PendingAck *pa = pending_alloc();
    if (pa) {
        pa->used    = true;
        pa->seq     = seq;
        pa->target  = target;
        pa->cmd     = CMD_PARAM_REL;
        pa->len     = 3;
        memcpy(pa->data, p, 3);
        pa->retries = 0;
        pa->t_sent  = millis();
    } else {
        log_add("Coda ACK piena", lv_color_hex(0xFFAA00));
    }

    lws_send_frame(Serial1, ID_DISPLAY, seq, CMD_PARAM_REL, p, 3);
}

// In comunicazioni.h, dopo send_param_reliable esistente:

// CMD_PARAM_VOCE: [target][voice][key][value]
static void send_param_voce(char target, uint8_t voice,
                            char key, uint8_t value) {
    uint8_t p[4] = { (uint8_t)target, voice, (uint8_t)key, value };
    lws_send_frame(Serial1, ID_DISPLAY, next_seq(),
                   CMD_PARAM_VOCE, p, 4);
}

// CMD_PARAM_I32_V: [target][voice][key][i32_le]
static void send_param_i32_voce(char target, uint8_t voice,
                                char key, int32_t value) {
    uint8_t p[7];
    p[0] = (uint8_t)target;
    p[1] = voice;
    p[2] = (uint8_t)key;
    lws_pack_i32_le(value, &p[3]);
    lws_send_frame(Serial1, ID_DISPLAY, next_seq(),
                   CMD_PARAM_I32_V, p, 7);
}

// CMD_MIDI_NOTE_V: [target][voice][onoff][pitch][vel]
static void send_midi_note_v(char target, uint8_t voice,
                             uint8_t onoff, uint8_t pitch, uint8_t vel) {
    uint8_t p[5] = { (uint8_t)target, voice, onoff, pitch, vel };
    lws_send_frame(Serial1, ID_DISPLAY, next_seq(),
                   CMD_MIDI_NOTE_V, p, 5);
}

static void send_error(char target, const char *error_msg) {
    uint8_t p[64];
    size_t l = strlen(error_msg);
    if (l > 62) l = 62;
    p[0] = (uint8_t)target;
    memcpy(&p[1], error_msg, l);
    lws_send_frame(Serial1, ID_DISPLAY, next_seq(), CMD_ERROR, p, 1 + (uint8_t)l);
}

// ACK per un frame ricevuto: payload = [acked_seq, acked_cmd]
static void send_param_ack(uint8_t acked_seq, uint8_t acked_cmd) {
    uint8_t p[2] = { acked_seq, acked_cmd };
    lws_send_frame(Serial1, ID_DISPLAY, next_seq(), CMD_PARAM_ACK, p, 2);
}

// ========================== RX ==========================
static void process_frame(const LwsFrame &f) {
    char sender_name[16] = "???";
    for (int i = 0; i < MAX_MCU; i++) {
        if (MCU_IDS[i] == (char)f.sender) { strcpy(sender_name, MCU_NAMES[i]); break; }
    }

    switch ((char)f.cmd) {
        case CMD_PING:
            send_pong();
            break;

        case CMD_PONG: {
            for (int i = 0; i < MAX_MCU; i++) {
                if (MCU_IDS[i] == (char)f.sender && !mcu_status.online[i]) {
                    mcu_status.online[i] = true;
                    mcu_status.ping_counter++;
                    break;
                }
            }
            log_add(sender_name, lv_color_hex(0x00FF00));
            break;
        }

        case CMD_PARAM_ACK: {
            if (f.len >= 2) {
                uint8_t acked_seq = f.data[0];
                uint8_t acked_cmd = f.data[1];
                PendingAck *pa = pending_find(acked_seq);
                if (pa && pa->cmd == acked_cmd) {
                    pa->used = false;
                    Serial.printf("ACK seq=%u cmd=%c from %s\n",
                                  acked_seq, (char)acked_cmd, sender_name);
                } else {
                    Serial.printf("ACK orfano seq=%u from %s\n",
                                  acked_seq, sender_name);
                }
            }
            break;
        }

        // Fire-and-forget: applica e basta, NESSUN ACK
        case CMD_PARAM: {
            if (f.len >= 3) {
                char    target = (char)f.data[0];
                char    key    = (char)f.data[1];
                uint8_t val    = f.data[2];
                if (target == 'A') {
                    for (uint8_t i = 0; i < MAPA_SIZE; i++)
                        if (mapA[i].key == key) { timbrA[presetNumA][mapA[i].index] = val; break; }
                } else if (target == 'B') {
                    for (uint8_t i = 0; i < MAPB_SIZE; i++)
                        if (mapB[i].key == key) { timbrB[presetNumB][mapB[i].index] = val; break; }
                }
				// in comunicazioni.h (Display), dentro process_frame, nei due case PARAM:
else if (target == ID_TEENSY) {
    // opzionale: aggiorna UI locale (slider BPM, label pattern, ecc.)
    Serial.printf("[T] key=%c val=%u\n", key, val);
}
            }
            break;
        }

        // Reliable: applica e invia ACK
        case CMD_PARAM_REL: {
            if (f.len >= 3) {
                char    target = (char)f.data[0];
                char    key    = (char)f.data[1];
                uint8_t val    = f.data[2];
                if (target == 'A') {
                    for (uint8_t i = 0; i < MAPA_SIZE; i++)
                        if (mapA[i].key == key) { timbrA[presetNumA][mapA[i].index] = val; break; }
                } else if (target == 'B') {
                    for (uint8_t i = 0; i < MAPB_SIZE; i++)
                        if (mapB[i].key == key) { timbrB[presetNumB][mapB[i].index] = val; break; }
                }
                send_param_ack(f.seq, f.cmd);
            }
            break;
        }

        case CMD_ERROR: {
            char msg[64] = {0};
            if (f.len >= 2) {
                size_t l = f.len - 1;
                if (l > 62) l = 62;
                memcpy(msg, &f.data[1], l);
                msg[l] = '\0';
            }
            log_add(msg, lv_color_hex(0xFF0000));
            for (int i = 0; i < MAX_MCU; i++){
                if (MCU_IDS[i] == (char)f.sender) { mcu_status.online[i] = false; break; }
		}
		}
            break;
			case CMD_TIMELINE: {
    if (f.len >= 8) {
        uint32_t cur = (uint32_t)lws_unpack_i32_le(&f.data[0]);
        uint32_t tot = (uint32_t)lws_unpack_i32_le(&f.data[4]);
        update_timeline(cur, tot);
    }
			}
    break;
	case CMD_MIDI_CC: {
            if (f.len >= 2) {
                Serial.printf("[MIDI] CC  %u = %u (from %s)\n",
                              f.data[0], f.data[1], sender_name);
            }
	}
            break;
        case CMD_MIDI_NOTE: {
            if (f.len >= 3) {
                Serial.printf("[MIDI] %s pitch=%u vel=%u (from %s)\n",
                              f.data[0] ? "NOTE_ON " : "NOTE_OFF",
                              f.data[1], f.data[2], sender_name);
            }   
        }
     break;
        case CMD_MIDI_BEND: {
            if (f.len >= 4) {
                int32_t bend = lws_unpack_i32_le(&f.data[0]);
                Serial.printf("[MIDI] BEND %ld (from %s)\n",
                              (long)bend, sender_name);
            }
           
        }
 break;
         case CMD_PRESET_ACK: {
            if (f.len >= 3) {
                uint8_t voice  = f.data[1];
                uint8_t status = f.data[2];
                preset_ack_received = true;
                preset_ack_status   = status;
                Serial.printf("[PRESET] ACK from %c voice=%u status=%u\n",
                              (char)f.sender, voice, status);
            }
            break;
        }
         // -----------------------------------------------------------------
        // DRUM_PATTERN: [ptn_num][name...]  (dal Teensy)
        // -----------------------------------------------------------------
        case CMD_DRUM_PATTERN: {
            if (f.len >= 1) {
                uint8_t ptn = f.data[0];
                char name[32] = {0};
                size_t nameLen = f.len - 1;
                if (nameLen > 31) nameLen = 31;
                memcpy(name, &f.data[1], nameLen);
                name[nameLen] = '\0';

                if (drum_pattern_label && lv_obj_is_valid(drum_pattern_label)) {
                    char buf[48];
                    // Se il nome è vuoto o uguale al default "PTN NN", mostra solo il numero
                    char defName[16];
                    snprintf(defName, sizeof(defName), "PTN %02u", ptn);

                    if (name[0] == '\0' || strcmp(name, defName) == 0)
                        snprintf(buf, sizeof(buf), "PTN %02u", ptn);
                    else
                        snprintf(buf, sizeof(buf), "PTN %02u: %s", ptn, name);

                    lv_label_set_text(drum_pattern_label, buf);
                }
                Serial.printf("[DRUM] ptn=%u name=\"%s\"\n", ptn, name);
            }
            break;
        }
    }
}

void leggiSer() {
    while (Serial1.available() > 0) {
        uint8_t b = (uint8_t)Serial1.read();
        LwsFrame f;
        if (lwsDisplayParser.feed(b, f)) process_frame(f);
    }
}

// ========================== POLL PER RETRY ACK ==========================
// DEVE essere chiamata ad ogni iterazione di loop().
void com_poll() {
    uint32_t now = millis();
    for (int i = 0; i < PENDING_MAX; i++) {
        PendingAck *pa = &g_pending[i];
        if (!pa->used) continue;
        if (now - pa->t_sent < ACK_TIMEOUT_MS) continue;

        if (pa->retries >= ACK_RETRIES_MAX) {
            char buf[48];
            snprintf(buf, sizeof(buf), "ACK timeout seq=%u target=%c",
                     pa->seq, pa->target);
            log_add(buf, lv_color_hex(0xFF0000));
            pa->used = false;
            continue;
        }

        pa->retries++;
        pa->t_sent = now;
        lws_send_frame(Serial1, ID_DISPLAY, pa->seq, pa->cmd, pa->data, pa->len);
        Serial.printf("Retry seq=%u (tent. %u)\n", pa->seq, pa->retries);
    }
}

// ========================== REPORT ==========================
static void print_mcu_status() {
    Serial.println("\n=== MCU DISCOVERY REPORT ===");
    int online_count = 0;
    for (int i = 0; i < MAX_MCU; i++) {
        if (MCU_IDS[i] == ID_DISPLAY) continue;   // salta se stesso
        Serial.print("- "); Serial.print(MCU_NAMES[i]); Serial.print(": ");
        if (mcu_status.online[i]) { Serial.println("ONLINE"); online_count++; }
        else                        Serial.println("OFFLINE");
    }
    Serial.print("TOTAL: "); Serial.print(online_count);
    Serial.print("/");       Serial.println(MAX_MCU - 1);
}

void resetPingStatus() {
    memset(&mcu_status, 0, sizeof(mcu_status));
    lwsDisplayParser.reset();
    memset(g_pending, 0, sizeof(g_pending));
}

// ========================== DISCOVERY (non bloccante, riavviabile) ==========================
enum : uint8_t {
    DISC_IDLE = 0,
    DISC_START,
    DISC_PING_SEND,
    DISC_WAIT,
    DISC_DONE
};

static uint8_t  g_disc_state       = DISC_IDLE;
static uint8_t  g_disc_ping_idx    = 0;
static uint32_t g_disc_t0          = 0;
static bool     g_disc_restart     = false;
static bool     g_discovery_active = false;

static const uint32_t DISC_START_DELAY_MS = 300;

static const char DISC_TARGETS[] = {
    ID_SYNTH_A1, ID_SYNTH_A2, ID_SYNTH_A3,
    ID_SYNTH_B,  ID_ROUTER,   ID_CTRL,
    ID_MOD,      ID_TEENSY,   ID_POWER
};
static const uint8_t DISC_TARGET_COUNT = sizeof(DISC_TARGETS);   // 9
static const uint8_t DISC_EXPECTED     = DISC_TARGET_COUNT;

// ---------- API pubblica ----------
bool discovery_active() {
    return g_discovery_active;
}

bool discovery_running() {
    return (g_disc_state != DISC_IDLE) && (g_disc_state != DISC_DONE);
}

void discover_all_mcu_start() {
    resetPingStatus();
    g_discovery_active = true;
    g_disc_ping_idx    = 0;
    g_disc_t0          = millis();
    g_disc_state       = DISC_START;
    g_disc_restart     = false;

    log_show();
    log_add("--- Discovery MCU ---", lv_color_hex(0xFFFF00));
}

void discover_request_restart() {
    if (discovery_running()) {
        log_add("Discovery gia' in corso", lv_color_hex(0xFFAA00));
        return;
    }
    g_disc_restart = true;
    log_show();
    log_add("Riavvio discovery...", lv_color_hex(0xFFFF00));
}

static void discover_all_mcu_finish() {
    print_mcu_status();

    if (mcu_status.ping_counter >= DISC_EXPECTED) {
        mcu_status.all_mcu_ok = true;
        log_add("Tutti gli MCU OK!", lv_color_hex(0x00FF00));
    } else {
        char buf[40];
        snprintf(buf, sizeof(buf), "Online: %u/%u",
                 (unsigned)mcu_status.ping_counter,
                 (unsigned)DISC_EXPECTED);
        log_add(buf, lv_color_hex(0xFFAA00));
    }

    g_discovery_active = false;
    g.log_t            = millis();
    g_disc_state       = DISC_DONE;
}

void discover_all_mcu_poll() {
    if (g_disc_restart) {
        g_disc_restart = false;
        discover_all_mcu_start();
        return;
    }

    switch (g_disc_state) {
        case DISC_IDLE:
        case DISC_DONE:
            return;

        case DISC_START:
            if (millis() - g_disc_t0 >= DISC_START_DELAY_MS)
                g_disc_state = DISC_PING_SEND;
            return;

        case DISC_PING_SEND:
            if (g_disc_ping_idx < DISC_TARGET_COUNT) {
                send_ping(DISC_TARGETS[g_disc_ping_idx]);
                g_disc_ping_idx++;
                return;
            }
            g_disc_t0    = millis();
            g_disc_state = DISC_WAIT;
            return;

        case DISC_WAIT:
            if (mcu_status.ping_counter >= DISC_EXPECTED) {
                discover_all_mcu_finish();
            } else if (millis() - g_disc_t0 >= PING_TIMEOUT_MS) {
                discover_all_mcu_finish();
            }
            return;
    }
}
// =========================================================================
// PRESET TRANSFER — API pubblica
// =========================================================================
// Richiede: SD montata, Serial1 attiva.
// Ritorna true se ACK OK, false se timeout/errore.

bool sendPresetToVoice(char target, uint8_t voice, uint8_t presetId);

// Variabili di stato (definite nel .ino)
extern volatile bool    preset_ack_received;
extern volatile uint8_t preset_ack_status;

#endif