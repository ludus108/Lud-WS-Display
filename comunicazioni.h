#ifndef COMUNICAZIONI_H
#define COMUNICAZIONI_H

#include <Arduino.h>
#include "globals.h"

// ========================== PROTOCOL CONSTANTS ==========================
#define FRAME_START     0x02  // STX (Start of Text)
#define FRAME_END       0x03  // ETX (End of Text)
#define FRAME_ESCAPE    0x1B  // ESC (per escape byte speciali)
#define MAX_FRAME_SIZE  64
#define PING_TIMEOUT_MS 2000  // Timeout per discovery iniziale
#define SERIAL_BAUDRATE 115200

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

#define MAX_MCU  9

// ========================== COMMAND CODES ==========================
#define CMD_PING        'p'   // Heartbeat/Discovery
#define CMD_PONG        'P'   // Ping response
#define CMD_PARAM       'S'   // Set parameter
#define CMD_GET_PARAM   'G'   // Get parameter request
#define CMD_PARAM_ACK   'A'   // Parameter acknowledge
#define CMD_ERROR       'E'   // Error message
#define CMD_STATUS      'Z'   // Status report

// ========================== FRAME STRUCTURE ==========================
struct Frame {
    uint8_t start;      // FRAME_START
    uint8_t sender;     // Sender MCU ID
    uint8_t cmd;        // Command code
    uint8_t target;     // Target MCU ID
    uint8_t len;        // Data length (0-60)
    uint8_t data[60];   // Payload data
    uint8_t crc;        // CRC8 checksum
    uint8_t end;        // FRAME_END
} __packed;

// ========================== PARAMETER MAPPING ==========================
struct ParamMapA { 
    char key; 
    uint8_t index; 
};

static const ParamMapA mapA[] = {
    {'a', wave_mode_A},    {'b', wave_A},        {'c', shape_A},         {'d', shape_lev_A},
    {'e', shape_rate_A},   {'f', lfo_pitch_lev_A}, {'g', cutOff_A},       {'h', res_A},
    {'i', vcf_lfo_A},      {'l', vcf_env_A},     {'m', vcf_ana_env_A},   {'n', ana_ATTACK_A},
    {'o', ana_DECAY_A},    {'p', ana_SUSTAIN_A}, {'q', ana_RELEASE_A},   {'r', vir_ATTACK_A},
    {'s', vir_DECAY_A},    {'t', vir_SUSTAIN_A}, {'u', vir_RELEASE_A},   {'v', lfo_wave_A},
    {'z', lfo_rate_A},     {'x', vca_vir_env_A}, {'y', vca_lfo_A}
};
static const uint8_t MAPA_SIZE = sizeof(mapA) / sizeof(mapA[0]);

struct ParamMapB { 
    char key; 
    uint8_t index; 
};

static const ParamMapB mapB[] = {
    {'a', wave_mode_B},    {'b', wave_B},        {'c', shape_B},         {'d', shape_lev_B},
    {'e', shape_rate_B},   {'f', lfo_pitch_lev_B}, {'g', vcf_mode_B},     {'h', cutOff_1_B},
    {'i', cutOff_2_B},     {'l', cutOff_3_B},    {'m', res_B},           {'n', vcf_lfo_B},
    {'o', vcf_env_B},      {'p', vcf_Bna_env_B}, {'q', ana_BTTACK_B},    {'r', ana_DECAY_B},
    {'s', ana_SUSTAIN_B},  {'t', ana_RELEASE_B}, {'u', vir_BTTACK_B},    {'v', vir_DECAY_B},
    {'z', vir_SUSTAIN_B},  {'x', vir_RELEASE_B}, {'y', lfo_wave_B},      {'w', lfo_rate_B},
    {'k', vca_vir_env_B},  {'j', vca_lfo_B}
};
static const uint8_t MAPB_SIZE = sizeof(mapB) / sizeof(mapB[0]);

// ========================== MCU STATUS TRACKING ==========================
static struct {
    bool online[MAX_MCU];
    uint8_t ping_counter;
    bool all_mcu_ok;
} mcu_status = {};

static const char* MCU_NAMES[MAX_MCU] = {
    "Display", "Synth A1", "Synth A2", "Synth A3",
    "Synth B", "Router", "Ctrl", "Mod", "Teensy"
};

static const char MCU_IDS[MAX_MCU] = {
    ID_DISPLAY, ID_SYNTH_A1, ID_SYNTH_A2, ID_SYNTH_A3,
    ID_SYNTH_B, ID_ROUTER, ID_CTRL, ID_MOD, ID_TEENSY
};

// ========================== CRC8 CALCULATION ==========================
/**
 * Calcola CRC8 Polynomial 0xD5 (standard per comunicazioni seriali)
 */
static uint8_t calculate_crc8(uint8_t *data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0xD5;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

// ========================== FRAME BUILDER ==========================
/**
 * Costruisce un frame completo con escape dei byte speciali
 * 
 * Formato frame:
 * [STX][Sender][Cmd][Target][Len][Data...][CRC][ETX]
 * 
 * Escape per byte speciali nel payload:
 * - Se data[i] == STX/ETX/ESC: [ESC][data[i] ^ 0xFF]
 */
static size_t build_frame(uint8_t *buffer, uint8_t sender, uint8_t cmd, 
                          uint8_t target, uint8_t *data, size_t data_len) {
    if (data_len > 60) data_len = 60;
    
    size_t pos = 0;
    
    // Start marker
    buffer[pos++] = FRAME_START;
    
    // Header (non escaped)
    buffer[pos++] = sender;
    buffer[pos++] = cmd;
    buffer[pos++] = target;
    buffer[pos++] = (uint8_t)data_len;
    
    // Payload con escape
    for (size_t i = 0; i < data_len; i++) {
        if (data[i] == FRAME_START || data[i] == FRAME_END || data[i] == FRAME_ESCAPE) {
            buffer[pos++] = FRAME_ESCAPE;
            buffer[pos++] = data[i] ^ 0xFF;
        } else {
            buffer[pos++] = data[i];
        }
    }
    
    // CRC su header + payload (originali, non escaped)
    uint8_t crc_data[4 + data_len];
    crc_data[0] = sender;
    crc_data[1] = cmd;
    crc_data[2] = target;
    crc_data[3] = (uint8_t)data_len;
    if (data_len > 0) {
        memcpy(&crc_data[4], data, data_len);
    }
    uint8_t crc = calculate_crc8(crc_data, 4 + data_len);
    buffer[pos++] = crc;
    
    // End marker
    buffer[pos++] = FRAME_END;
    
    return pos;
}

// ========================== FRAME PARSER ==========================
static uint8_t frame_buffer[MAX_FRAME_SIZE];
static size_t frame_pos = 0;
static bool frame_in_progress = false;
static bool escape_next = false;

/**
 * Parser robusto con escape handling
 * Ritorna true se frame completo ricevuto
 */
static bool parse_frame_byte(uint8_t byte, Frame *out_frame) {
    // Cerca start marker
    if (!frame_in_progress && byte == FRAME_START) {
        frame_pos = 0;
        frame_in_progress = true;
        escape_next = false;
        return false;
    }
    
    if (!frame_in_progress) return false;
    
    // Gestione escape
    if (escape_next) {
        frame_buffer[frame_pos++] = byte ^ 0xFF;
        escape_next = false;
        return false;
    }
    
    if (byte == FRAME_ESCAPE) {
        escape_next = true;
        return false;
    }
    
    // End frame
    if (byte == FRAME_END) {
        if (frame_pos < 5) {  // Minimo: sender+cmd+target+len+crc
            frame_pos = 0;
            frame_in_progress = false;
            escape_next = false;
            return false;
        }
        
        // Verifica CRC
        uint8_t received_crc = frame_buffer[frame_pos - 1];
        uint8_t calculated_crc = calculate_crc8(frame_buffer, frame_pos - 1);
        
        if (received_crc != calculated_crc) {
            Serial.print("❌ CRC error (got ");
            Serial.print(received_crc);
            Serial.print(" expected ");
            Serial.print(calculated_crc);
            Serial.println(")");
            frame_pos = 0;
            frame_in_progress = false;
            escape_next = false;
            return false;
        }
        
        // Parse frame valido
        out_frame->sender = frame_buffer[0];
        out_frame->cmd = frame_buffer[1];
        out_frame->target = frame_buffer[2];
        out_frame->len = frame_buffer[3];
        
        if (out_frame->len > 60) out_frame->len = 60;
        memcpy(out_frame->data, &frame_buffer[4], out_frame->len);
        out_frame->crc = received_crc;
        
        frame_pos = 0;
        frame_in_progress = false;
        escape_next = false;
        return true;
    }
    
    // Accumula byte
    if (frame_pos < MAX_FRAME_SIZE - 1) {
        frame_buffer[frame_pos++] = byte;
    } else {
        Serial.println("❌ Buffer overflow");
        frame_pos = 0;
        frame_in_progress = false;
        escape_next = false;
        return false;
    }
    
    return false;
}

// ========================== TRANSMISSION FUNCTIONS ==========================

/**
 * Invia PING per discovery (solo in setup)
 */
static void send_ping(char target_mcu) {
    uint8_t data[1] = {0};
    uint8_t buffer[MAX_FRAME_SIZE];
    size_t len = build_frame(buffer, ID_DISPLAY, CMD_PING, target_mcu, data, 0);
    
    Serial1.write(buffer, len);
    
    // Debug
    for (int i = 0; i < MAX_MCU; i++) {
        if (MCU_IDS[i] == target_mcu) {
            Serial.print("📡 Ping → ");
            Serial.println(MCU_NAMES[i]);
            return;
        }
    }
    Serial.print("📡 Ping → MCU ");
    Serial.println((char)target_mcu);
}

/**
 * Invia PONG response
 */
static void send_pong(char target_mcu) {
    uint8_t data[1] = {0};
    uint8_t buffer[MAX_FRAME_SIZE];
    size_t len = build_frame(buffer, ID_DISPLAY, CMD_PONG, target_mcu, data, 0);
    Serial1.write(buffer, len);
}

/**
 * Invia SET PARAMETER
 * target: 'A' o 'B' (Synth A o B)
 * param_key: chiave del parametro (a-z)
 * value: valore 0-255
 */
static void send_param_update(char target, char param_key, uint8_t value) {
    uint8_t data[2] = {(uint8_t)param_key, value};
    uint8_t buffer[MAX_FRAME_SIZE];
    size_t len = build_frame(buffer, ID_DISPLAY, CMD_PARAM, target, data, 2);
    Serial1.write(buffer, len);
    
    Serial.print("📤 Param: target=");
    Serial.print(target);
    Serial.print(" key=");
    Serial.print(param_key);
    Serial.print(" val=");
    Serial.println(value);
}

/**
 * Invia ERROR report
 */
static void send_error(char target, const char *error_msg) {
    uint8_t data[60];
    size_t len = strlen(error_msg);
    if (len > 60) len = 60;
    memcpy(data, (uint8_t*)error_msg, len);
    
    uint8_t buffer[MAX_FRAME_SIZE];
    size_t frame_len = build_frame(buffer, ID_DISPLAY, CMD_ERROR, target, data, len);
    Serial1.write(buffer, frame_len);
}

// ========================== RECEIVING & PROCESSING ==========================

/**
 * Elabora frame ricevuto
 */
static void process_frame(Frame *frame) {
    char sender_name[16] = "???";
    for (int i = 0; i < MAX_MCU; i++) {
        if (MCU_IDS[i] == frame->sender) {
            strcpy(sender_name, MCU_NAMES[i]);
            break;
        }
    }
    
    switch (frame->cmd) {
        case CMD_PONG: {
            // MCU risponde al ping (discovery)
            Serial.print("✅ PONG from ");
            Serial.println(sender_name);
            
            // Mark MCU as online
            for (int i = 0; i < MAX_MCU; i++) {
                if (MCU_IDS[i] == frame->sender) {
                    mcu_status.online[i] = true;
                    mcu_status.ping_counter++;
                    break;
                }
            }
            
            log_add(sender_name, lv_color_hex(0x00FF00));
            
            // Controlla se tutti gli MCU rispondono
            if (mcu_status.ping_counter >= (MAX_MCU - 1)) {  // -1 escludi Display
                mcu_status.all_mcu_ok = true;
                Serial.println("🎉 Tutti gli MCU sono online!");
                log_add("Tutti gli MCU OK!", lv_color_hex(0x00FF00));
            }
            break;
        }
        
        case CMD_PARAM_ACK: {
            // Synth ha confermato ricezione parametro
            Serial.print("✔️ Param ACK from ");
            Serial.println(sender_name);
            break;
        }
        
        case CMD_PARAM: {
            // Synth invia aggiornamento parametro (feedback)
            if (frame->len >= 2) {
                char param_key = (char)frame->data[0];
                uint8_t value = frame->data[1];
                char target = frame->target;
                
                Serial.print("📥 Param from ");
                Serial.print(sender_name);
                Serial.print(": ");
                Serial.print(param_key);
                Serial.print("=");
                Serial.println(value);
                
                // Aggiorna preset locale
                if (target == 'A') {
                    for (uint8_t i = 0; i < MAPA_SIZE; i++) {
                        if (mapA[i].key == param_key) {
                            timbrA[presetNumA][mapA[i].index] = value;
                            break;
                        }
                    }
                } else if (target == 'B') {
                    for (uint8_t i = 0; i < MAPB_SIZE; i++) {
                        if (mapB[i].key == param_key) {
                            timbrB[presetNumB][mapB[i].index] = value;
                            break;
                        }
                    }
                }
            }
            break;
        }
        
        case CMD_ERROR: {
            // Errore ricevuto da MCU
            char error_msg[61];
            if (frame->len > 60) frame->len = 60;
            memcpy(error_msg, frame->data, frame->len);
            error_msg[frame->len] = '\0';
            
            Serial.print("❌ ERROR from ");
            Serial.print(sender_name);
            Serial.print(": ");
            Serial.println(error_msg);
            log_add(error_msg, lv_color_hex(0xFF0000));
            
            // Marca MCU come offline se troppi errori
            for (int i = 0; i < MAX_MCU; i++) {
                if (MCU_IDS[i] == frame->sender) {
                    mcu_status.online[i] = false;
                    break;
                }
            }
            break;
        }
        
        case CMD_STATUS: {
            Serial.print("📊 STATUS from ");
            Serial.println(sender_name);
            break;
        }
        
        default:
            Serial.print("❓ Unknown command (");
            Serial.print((char)frame->cmd);
            Serial.print(") from ");
            Serial.println(sender_name);
            break;
    }
}

/**
 * Main serial reader - call in loop()
 * Processa un byte alla volta (non-bloccante)
 */
void leggiSer() {
    static Frame current_frame;
    
    while (Serial1.available() > 0) {
        uint8_t byte = Serial1.read();
        
        if (parse_frame_byte(byte, &current_frame)) {
            process_frame(&current_frame);
        }
    }
}

/**
 * Invia ping a tutti gli MCU (SETUP ONLY)
 * Aspetta risposte per PING_TIMEOUT_MS
 */
static void discover_all_mcu() {
    resetPingStatus();
    
    // Invia ping a tutti i synth/devices (escluso Display)
    send_ping('a');  // Synth A1
    send_ping('b');  // Synth A2
    send_ping('c');  // Synth A3
    send_ping('B');  // Synth B
    send_ping('R');  // Router
    send_ping('C');  // Ctrl
    send_ping('M');  // Mod
    send_ping('T');  // Teensy
    send_ping('P');  // Power (se implementato)
    
    // Aspetta risposte per il timeout
    uint32_t start = millis();
    while (millis() - start < PING_TIMEOUT_MS) {
        leggiSer();
        delay(1);
    }
    
    // Report finale
    print_mcu_status();
}

/**
 * Inizializza status MCU
 */
void resetPingStatus() {
    memset(&mcu_status, 0, sizeof(mcu_status));
    mcu_status.ping_counter = 0;
    mcu_status.all_mcu_ok = false;
}

/**
 * Ritorna true se MCU è online
 */
static bool is_mcu_online(char mcu_id) {
    for (int i = 0; i < MAX_MCU; i++) {
        if (MCU_IDS[i] == mcu_id) {
            return mcu_status.online[i];
        }
    }
    return false;
}

/**
 * Debug: stampa status di tutti gli MCU
 */
static void print_mcu_status() {
    Serial.println("\n╔════════════════════════════════════╗");
    Serial.println("║         MCU DISCOVERY REPORT       ║");
    Serial.println("╠════════════════════════════════════╣");
    
    int online_count = 0;
    for (int i = 0; i < MAX_MCU; i++) {
        Serial.print("║ ");
        Serial.print(MCU_NAMES[i]);
        for (int j = strlen(MCU_NAMES[i]); j < 20; j++) Serial.print(" ");
        
        if (mcu_status.online[i]) {
            Serial.println("  ✅ ONLINE   ║");
            online_count++;
        } else {
            Serial.println("  ❌ OFFLINE  ║");
        }
    }
    
    Serial.println("╠════════════════════════════════════╣");
    Serial.print("║ TOTAL: ");
    Serial.print(online_count);
    Serial.print("/");
    Serial.print(MAX_MCU);
    Serial.println(" MCU online        ║");
    Serial.println("╚════════════════════════════════════╝\n");
}

#endif
