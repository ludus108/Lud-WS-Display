#ifndef COMUNICAZIONI_H
#define COMUNICAZIONI_H

#include <Arduino.h>
#include "globals.h"
// ========================== IDENTIFICATORI MCU ==========================
#define ID_DISPLAY  'D'
#define ID_SYNTH_A1 'a'
#define ID_SYNTH_A2 'b'
#define ID_SYNTH_A3 'c'
#define ID_SYNTH_B  'B'
#define ID_ROUTER   'R'
#define ID_CTRL     'C'
#define ID_MOD      'M'
#define ID_TEENSY   'T'

#define MAX_MCU  8
#define SER_BUFF_SIZE 64

// ========================== VARIABILI ESTERNE ==========================
extern int presetNumA;
extern int presetNumB;
extern int timbrA[16][23];
extern int timbrB[16][26];

// ========================== MAPPATURA PARAMETRI ==========================
struct ParamMapA { char key; uint8_t index; };
static const ParamMapA mapA[] = {
    {'a', wave_mode_A}, {'b', wave_A}, {'c', shape_A}, {'d', shape_lev_A},
    {'e', shape_rate_A}, {'f', lfo_pitch_lev_A}, {'g', cutOff_A}, {'h', res_A},
    {'i', vcf_lfo_A}, {'l', vcf_env_A}, {'m', vcf_ana_env_A}, {'n', ana_ATTACK_A},
    {'o', ana_DECAY_A}, {'p', ana_SUSTAIN_A}, {'q', ana_RELEASE_A}, {'r', vir_ATTACK_A},
    {'s', vir_DECAY_A}, {'t', vir_SUSTAIN_A}, {'u', vir_RELEASE_A}, {'v', lfo_wave_A},
    {'z', lfo_rate_A}, {'x', vca_vir_env_A}, {'y', vca_lfo_A}
};
static const uint8_t MAPA_SIZE = sizeof(mapA)/sizeof(mapA[0]);

struct ParamMapB { char key; uint8_t index; };
static const ParamMapB mapB[] = {
    {'a', wave_mode_B}, {'b', wave_B}, {'c', shape_B}, {'d', shape_lev_B},
    {'e', shape_rate_B}, {'f', lfo_pitch_lev_B}, {'g', vcf_mode_B}, {'h', cutOff_1_B},
    {'i', cutOff_2_B}, {'l', cutOff_3_B}, {'m', res_B}, {'n', vcf_lfo_B},
    {'o', vcf_env_B}, {'p', vcf_Bna_env_B}, {'q', ana_BTTACK_B}, {'r', ana_DECAY_B},
    {'s', ana_SUSTAIN_B}, {'t', ana_RELEASE_B}, {'u', vir_BTTACK_B}, {'v', vir_DECAY_B},
    {'z', vir_SUSTAIN_B}, {'x', vir_RELEASE_B}, {'y', lfo_wave_B}, {'w', lfo_rate_B},
    {'k', vca_vir_env_B}, {'j', vca_lfo_B}
};
static const uint8_t MAPB_SIZE = sizeof(mapB)/sizeof(mapB[0]);

// ========================== PING ==========================
static uint8_t pingCounter = 0;
static bool allMCUok = false;

void sendPing(char targetMCU) {
    Serial1.write(targetMCU);
    Serial1.write('p');
    Serial1.write("&!");
    Serial.print("send ping ");
    Serial.println(targetMCU);
}

void resetPingStatus() {
    pingCounter = 0;
    allMCUok = false;
}

static void onPingOK(char mcuID) {
    const char* name;
    switch (mcuID) {
        case ID_SYNTH_A1: name = "Synth A1"; break;
        case ID_SYNTH_A2: name = "Synth A2"; break;
        case ID_SYNTH_A3: name = "Synth A3"; break;
        case ID_SYNTH_B:  name = "Synth B";  break;
        case ID_ROUTER:   name = "Router";   break;
        case ID_CTRL:     name = "CTRL";     break;
        case ID_MOD:      name = "MOD";      break;
        case ID_TEENSY:   name = "Teensy";   break;
        default:          name = "???";      break;
    }
    Serial.print("MCU ");
    Serial.print(name);
    Serial.println(" OK");
    log_add(name, lv_color_hex(0x00FF00));

    pingCounter++;
    if (pingCounter >= MAX_MCU) {
        allMCUok = true;
        Serial.println("Tutti gli MCU sono online!");
        log_add("Tutti gli MCU OK!", lv_color_hex(0x00FF00));
    }
}

// ========================== PARSER ==========================
static char serBuff[SER_BUFF_SIZE];
static size_t posId = 0;

void leggiSer() {
    while (Serial1.available() > 0) {
        char c = Serial1.read();
        if (c == '!' && posId >= 1 && serBuff[posId - 1] == '&') {
            serBuff[posId - 1] = '\0';
            serBuff[posId] = '\0';

            if (posId >= 3) {
                char sender = serBuff[0];
                char command = serBuff[1];

                if (command == 'p') {
                    if (sender == ID_DISPLAY) {
                        onPingOK(serBuff[2]);
                    }
                }
                else if (command == 'P' && (serBuff[2] == 'A' || serBuff[2] == 'B')) {
                    char target = serBuff[2];
                    char paramKey = serBuff[3];
                    uint8_t value = (uint8_t)serBuff[4];

                    if (target == 'A') {
                        for (uint8_t i = 0; i < MAPA_SIZE; i++) {
                            if (mapA[i].key == paramKey) {
                                timbrA[presetNumA][mapA[i].index] = value;
                                break;
                            }
                        }
                    } else { // target == 'B'
                        for (uint8_t i = 0; i < MAPB_SIZE; i++) {
                            if (mapB[i].key == paramKey) {
                                timbrB[presetNumB][mapB[i].index] = value;
                                break;
                            }
                        }
                    }
                }
            }
            posId = 0;
        }
        else {
            if (posId < SER_BUFF_SIZE - 1) {
                serBuff[posId++] = c;
            } else {
                posId = 0;
            }
        }
    }
}

#endif