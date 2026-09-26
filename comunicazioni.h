#ifndef COMUNICAZIONI_H
#define COMUNICAZIONI_H

/*
 * ============================================================================
 *  comunicazioni.h — LWSv1.1 (LUD-WS Serial Protocol v1.1) — Display
 * ============================================================================
 *
 *  Contiene SOLO dichiarazioni (prototipi + extern).
 *  Le definizioni sono in comunicazioni.cpp.
 *
 *  Formato frame:
 *      [SENDER][SEQ][CMD][LEN][PAYLOAD...][CRC8][&][!]
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

#define MAX_MCU         10
#define PING_TIMEOUT_MS 2000

// ========================== PRESET TRANSFER ==========================
// Variabili definite in Lud-WS-Display.ino
extern volatile bool    preset_ack_received;
extern volatile uint8_t preset_ack_status;

// ========================== API PUBBLICA ==========================
// --- TX ---
void send_param_update(char target, char param_key, uint8_t value);
void send_param_reliable(char target, char param_key, uint8_t value);
void send_param_voce(char target, uint8_t voice, char key, uint8_t value);
void send_param_i32_voce(char target, uint8_t voice, char key, int32_t value);
void send_midi_note_v(char target, uint8_t voice,
                      uint8_t onoff, uint8_t pitch, uint8_t vel);
void send_error(char target, const char *error_msg);
char targetForVoice(uint8_t voice);
// --- SEQ TX (usata anche da preset_transfer.h) ---
uint8_t next_seq();

// --- RX + polling ---
void leggiSer();
void com_poll();

// --- Discovery ---
void resetPingStatus();
void discover_all_mcu_start();
void discover_all_mcu_poll();
void discover_request_restart();
bool discovery_active();
bool discovery_running();

#endif // COMUNICAZIONI_H