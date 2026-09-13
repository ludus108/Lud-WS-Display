#ifndef COMUNICAZIONI_H
#define COMUNICAZIONI_H

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

#define MAX_MCU         9
#define PING_TIMEOUT_MS 2000

// ========================== COMMAND CODES (LWSv1) ==========================
#define CMD_PING        'p'
#define CMD_PONG        'P'
#define CMD_PARAM       'S'
#define CMD_GET_PARAM   'G'
#define CMD_PARAM_ACK   'A'
#define CMD_ERROR       'E'
#define CMD_STATUS      'Z'

// ========================== PARAMETER MAPPING ==========================
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
    "Synth B", "Router",   "Ctrl",     "Mod",     "Teensy"
};
static const char MCU_IDS[MAX_MCU] = {
    ID_DISPLAY, ID_SYNTH_A1, ID_SYNTH_A2, ID_SYNTH_A3,
    ID_SYNTH_B, ID_ROUTER,   ID_CTRL,     ID_MOD,     ID_TEENSY
};

static LwsParser lwsDisplayParser;

// ========================== TX ==========================
static void send_ping(char target_mcu) {
    uint8_t p[1] = { (uint8_t)target_mcu };
    lws_send_frame(Serial1, ID_DISPLAY, CMD_PING, p, 1);
    for (int i = 0; i < MAX_MCU; i++) {
        if (MCU_IDS[i] == target_mcu) {
            Serial.print("Ping -> "); Serial.println(MCU_NAMES[i]);
            return;
        }
    }
}

static void send_pong() {
    lws_send_frame(Serial1, ID_DISPLAY, CMD_PONG, nullptr, 0);
}

static void send_param_update(char target, char param_key, uint8_t value) {
    uint8_t p[3] = { (uint8_t)target, (uint8_t)param_key, value };
    lws_send_frame(Serial1, ID_DISPLAY, CMD_PARAM, p, 3);
}

static void send_error(char target, const char *error_msg) {
    uint8_t p[64];
    size_t l = strlen(error_msg);
    if (l > 62) l = 62;
    p[0] = (uint8_t)target;
    memcpy(&p[1], error_msg, l);
    lws_send_frame(Serial1, ID_DISPLAY, CMD_ERROR, p, 1 + (uint8_t)l);
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

        case CMD_PARAM_ACK:
            Serial.print("Param ACK from "); Serial.println(sender_name);
            break;

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
            for (int i = 0; i < MAX_MCU; i++)
                if (MCU_IDS[i] == (char)f.sender) { mcu_status.online[i] = false; break; }
            break;
        }

        default:
            break;
    }
}

void leggiSer() {
    while (Serial1.available() > 0) {
        uint8_t b = (uint8_t)Serial1.read();
        LwsFrame f;
        if (lwsDisplayParser.feed(b, f)) process_frame(f);
    }
}

static void print_mcu_status() {
    Serial.println("\n=== MCU DISCOVERY REPORT ===");
    int online_count = 0;
    for (int i = 0; i < MAX_MCU; i++) {
        Serial.print("- "); Serial.print(MCU_NAMES[i]); Serial.print(": ");
        if (mcu_status.online[i]) { Serial.println("ONLINE"); online_count++; }
        else                        Serial.println("OFFLINE");
    }
    Serial.print("TOTAL: "); Serial.print(online_count);
    Serial.print("/");       Serial.println(MAX_MCU);
}

void resetPingStatus() {
    memset(&mcu_status, 0, sizeof(mcu_status));
    lwsDisplayParser.reset();
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
static const uint8_t DISC_TARGET_COUNT = sizeof(DISC_TARGETS);
static const uint8_t DISC_EXPECTED     = DISC_TARGET_COUNT; // 9

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

#endif